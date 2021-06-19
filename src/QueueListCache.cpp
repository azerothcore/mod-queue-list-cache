/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#include "QueueListCache.h"
#include "ArenaTeamMgr.h"
#include "BattlegroundMgr.h"
#include "BattlegroundQueue.h"
#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "Log.h"
#include "Optional.h"
#include "TaskScheduler.h"

namespace
{
    using QueueArenaRatedTemplate = std::tuple<
        char const*, // team name
        uint8, // arena type
        uint32>; // team rating

    using QueueArenaNonRatedTemplate = std::tuple<
        std::string, // arena type
        uint32, // level min
        uint32, // level max
        uint32, // queued total
        uint32>; // players need

    using QueueBgTemplate = std::tuple<
        char const*, // bg name
        uint32, // level min
        uint32, // level max
        uint32, // queued total
        uint32>; // players need

    // Core queue arena
    std::vector<QueueArenaRatedTemplate> queueArenaRatedList;
    std::vector<QueueArenaNonRatedTemplate> queueArenaNonRatedList;

    // Core queue bg
    std::vector<QueueBgTemplate> queueBgNormalList;
    std::vector<QueueBgTemplate> queueBgPremadeList;

    // Modules queue bg
    std::vector<QueueBgTemplate> queueCFBGList; // mod-cfbg

    TaskScheduler scheduler;

    bool _isEnableSystem = false;
}

QueueListCache* QueueListCache::instance()
{
    static QueueListCache instance;
    return &instance;
}

void QueueListCache::Init(bool reload /*= false*/)
{
    _isEnableSystem = sConfigMgr->GetOption<bool>("QLC.Enable", false);

    if (!_isEnableSystem && !reload)
    {
        // Skip loading if module disable
        LOG_INFO("server.loading", "> System disable");
        return;
    }

    // Add support for reload config
    scheduler.CancelAll();

    // Make new task for update all cache
    scheduler.Schedule(Seconds(sConfigMgr->GetOption<uint32>("QLC.Update.Delay", 5)), [&](TaskContext context)
    {
        UpdateArenaRated();
        UpdateArenaNonRated();
        UpdateBg();

        context.Repeat();
    });

    if (!reload)
    {
        // Added info about loading system
        LOG_INFO("server.loading", "> System loaded");
    }
}

void QueueListCache::Update(uint32 diff)
{
    if (!_isEnableSystem)
    {
        // Skip update if module disable
        return;
    }

    scheduler.Update(diff);
}

void QueueListCache::UpdateArenaRated()
{
    queueArenaRatedList.clear();

    // Rated arena
    for (uint32 qtype = BATTLEGROUND_QUEUE_2v2; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
    {
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(BattlegroundQueueTypeId(qtype));

        for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
        {
            BattlegroundBracketId bracketID = BattlegroundBracketId(bracket);

            // Skip if all empty
            if (bgQueue.IsAllQueuesEmpty(bracketID))
                continue;

            for (uint8 ii = BG_QUEUE_PREMADE_ALLIANCE; ii <= BG_QUEUE_PREMADE_HORDE; ii++)
            {
                for (auto const& itr : bgQueue.m_QueuedGroups[bracketID][ii])
                {
                    if (!itr->IsRated)
                        continue;

                    ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(itr->ArenaTeamId);
                    if (!team)
                        continue;

                    queueArenaRatedList.emplace_back(team->GetName().c_str(), itr->ArenaType, itr->ArenaTeamRating);
                }
            }
        }
    }
}

void QueueListCache::UpdateArenaNonRated()
{
    queueArenaNonRatedList.clear();

    auto GetTemplate = [&](BattlegroundQueue& bgQueue, BattlegroundBracketId bracketID) -> Optional<QueueArenaNonRatedTemplate>
    {
        Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgQueue.GetBGTypeID());
        if (!bg || bg->isBattleground())
            return std::nullopt;

        PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg->GetMapId(), bracketID);
        if (!bracketEntry)
            return std::nullopt;

        uint8 arenaType = bgQueue.GetArenaType();

        if (!arenaType)
            return std::nullopt;

        auto arenatype = Acore::StringFormat("%uv%u", arenaType, arenaType);
        uint32 playersNeed = ArenaTeam::GetReqPlayersForType(arenaType);
        uint32 minLevel = std::min(bracketEntry->minLevel, (uint32)80);
        uint32 maxLevel = std::min(bracketEntry->maxLevel, (uint32)80);
        uint32 qPlayers = bgQueue.GetPlayersCountInGroupsQueue(bracketID, BG_QUEUE_NORMAL_HORDE) + bgQueue.GetPlayersCountInGroupsQueue(bracketID, BG_QUEUE_NORMAL_ALLIANCE);

        if (!qPlayers)
            return std::nullopt;

        return std::make_tuple(arenatype, minLevel, maxLevel, qPlayers, playersNeed);
    };

    // Non rated arena
    for (uint32 qtype = BATTLEGROUND_QUEUE_2v2; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
    {
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(BattlegroundQueueTypeId(qtype));

        for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
        {
            BattlegroundBracketId bracketID = BattlegroundBracketId(bracket);

            // Skip if all empty
            if (bgQueue.IsAllQueuesEmpty(bracketID))
                continue;

            auto _queueList = GetTemplate(bgQueue, bracketID);
            if (_queueList)
                queueArenaNonRatedList.emplace_back(*_queueList);
        }
    }
}

void QueueListCache::UpdateBg()
{
    queueBgNormalList.clear();
    queueBgPremadeList.clear();
    queueCFBGList.clear();

    auto GetTemplate = [&](BattlegroundQueue& bgQueue, BattlegroundBracketId bracketID,
        BattlegroundQueueGroupTypes queueHorde, BattlegroundQueueGroupTypes queueAlliance) -> Optional<QueueBgTemplate>
    {
        Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgQueue.GetBGTypeID());
        if (!bg || bg->isArena())
            return std::nullopt;

        PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg->GetMapId(), bracketID);
        if (!bracketEntry)
            return std::nullopt;

        char const* bgName = bg->GetName();
        uint32 MinPlayers = bg->GetMinPlayersPerTeam();
        uint32 MaxPlayers = MinPlayers * 2;
        uint32 q_min_level = std::min(bracketEntry->minLevel, (uint32)80);
        uint32 q_max_level = std::min(bracketEntry->maxLevel, (uint32)80);
        uint32 qHorde = bgQueue.GetPlayersCountInGroupsQueue(bracketID, queueHorde);
        uint32 qAlliance = bgQueue.GetPlayersCountInGroupsQueue(bracketID, queueAlliance);
        auto qTotal = qHorde + qAlliance;

        if (!qTotal)
            return std::nullopt;

        return std::make_tuple(bgName, q_min_level, q_max_level, qTotal, MaxPlayers);
    };

    auto GetTemplateCFBG = [&](BattlegroundQueue& bgQueue, BattlegroundBracketId bracketID, BattlegroundQueueGroupTypes queueType) -> Optional<QueueBgTemplate>
    {
        Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgQueue.GetBGTypeID());
        if (!bg || bg->isArena())
            return std::nullopt;

        PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg->GetMapId(), bracketID);
        if (!bracketEntry)
            return std::nullopt;

        char const* bgName = bg->GetName();
        uint32 MinPlayers = bg->GetMinPlayersPerTeam();
        uint32 MaxPlayers = MinPlayers * 2;
        uint32 q_min_level = std::min(bracketEntry->minLevel, (uint32)80);
        uint32 q_max_level = std::min(bracketEntry->maxLevel, (uint32)80);
        auto qTotal = bgQueue.GetPlayersCountInGroupsQueue(bracketID, queueType);

        if (!qTotal)
            return std::nullopt;

        return std::make_tuple(bgName, q_min_level, q_max_level, qTotal, MaxPlayers);
    };

    for (uint32 qtype = BATTLEGROUND_QUEUE_AV; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
    {
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(BattlegroundQueueTypeId(qtype));

        for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
        {
            BattlegroundBracketId bracketID = BattlegroundBracketId(bracket);

            // Skip if all empty
            if (bgQueue.IsAllQueuesEmpty(bracketID))
                continue;

            Optional<QueueBgTemplate> _queueListNormal = GetTemplate(bgQueue, bracketID, BG_QUEUE_NORMAL_HORDE, BG_QUEUE_NORMAL_ALLIANCE);
            Optional<QueueBgTemplate> _queueListPremade = GetTemplate(bgQueue, bracketID, BG_QUEUE_PREMADE_HORDE, BG_QUEUE_PREMADE_ALLIANCE);
            Optional<QueueBgTemplate> _queueListCFBG = GetTemplateCFBG(bgQueue, bracketID, BattlegroundQueueGroupTypes(4));

            if (_queueListNormal)
                queueBgNormalList.emplace_back(*_queueListNormal);

            if (_queueListPremade)
                queueBgPremadeList.emplace_back(*_queueListPremade);

            if (_queueListCFBG)
                queueCFBGList.emplace_back(*_queueListCFBG);
        }
    }
}

void QueueListCache::ShowArenaRated(ChatHandler* handler)
{
    if (!_isEnableSystem)
    {
        handler->SendSysMessage("# Module for check queue is disabled");
        return;
    }

    if (queueArenaRatedList.empty())
    {
        handler->SendSysMessage("> All queues empty");
    }
    else
    {
        handler->SendSysMessage("# Queue status for rated arena:");

        for (auto const& [teamName, arenaType, teamRating] : queueArenaRatedList)
        {
            // Queue status
            handler->PSendSysMessage("> %s (%uv%u): %u", teamName, arenaType, arenaType, teamRating);
        }
    }
}

void QueueListCache::ShowArenaNonRated(ChatHandler* handler)
{
    if (!_isEnableSystem)
    {
        handler->SendSysMessage("# Module for check queue is disabled");
        return;
    }

    if (queueArenaNonRatedList.empty())
    {
        handler->SendSysMessage("> All queues empty");
    }
    else
    {
        handler->SendSysMessage("# Queue status for non rated arena:");

        for (auto const& [arenaType, minLevel, maxLevel, qTotal, MaxPlayers] : queueArenaNonRatedList)
        {
            // Queue status
            handler->PSendSysMessage("> %u-%u %s: %u/%u", minLevel, maxLevel, arenaType.c_str(), qTotal, MaxPlayers);
        }
    }
}

void QueueListCache::ShowBg(ChatHandler* handler)
{
    if (!_isEnableSystem)
    {
        handler->SendSysMessage("# Module for check queue is disabled");
        return;
    }

    auto ShowQueueStatus = [&](std::vector<QueueBgTemplate>& listQueue)
    {
        if (listQueue.empty())
            return;

        for (auto const& [bgName, minLevel, maxLevel, qTotal, MaxPlayers] : listQueue)
        {
            // Queue status
            handler->PSendSysMessage("> %u-%u %s: %u/%u", minLevel, maxLevel, bgName, qTotal, MaxPlayers);
        }
    };

    if (queueBgNormalList.empty() && queueBgPremadeList.empty() && queueCFBGList.empty())
    {
        handler->SendSysMessage("> All queues empty");
    }

    // If mod-cfbg enale, show only cfbg queue
    if (!queueCFBGList.empty())
    {
        handler->SendSysMessage("# Queue status for cross faction battleground:");
        ShowQueueStatus(queueCFBGList);
    }
    else if (!queueBgNormalList.empty() || !queueBgPremadeList.empty())
    {
        handler->SendSysMessage("# Queue status for battleground:");
        ShowQueueStatus(queueBgNormalList);
        ShowQueueStatus(queueBgPremadeList);
    }
}
