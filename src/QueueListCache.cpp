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
    scheduler.Schedule(Seconds(sConfigMgr->GetOption<uint32>("QLC.Update.Delay", 5)), [this](TaskContext context)
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

                    queueArenaRatedList.emplace_back(team->GetName(), itr->ArenaType, itr->ArenaTeamRating);
                }
            }
        }
    }
}

void QueueListCache::UpdateArenaNonRated()
{
    queueArenaNonRatedList.clear();

    auto AddCache = [this](BattlegroundQueueTypeId qtype, BattlegroundBracketId bracket)
    {
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(qtype);

        // Skip if all empty
        if (bgQueue.IsAllQueuesEmpty(bracket))
            return;

        for (uint8 ii = BG_QUEUE_NORMAL_ALLIANCE; ii <= BG_QUEUE_NORMAL_HORDE; ii++)
        {
            for (auto const& groupInfo : bgQueue.m_QueuedGroups[bracket][ii])
            {
                Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(groupInfo->BgTypeId));
                if (!bg || bg->isBattleground())
                    continue;

                PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg->GetMapId(), BattlegroundBracketId(groupInfo->BracketId));
                if (!bracketEntry)
                    continue;

                auto arenaType = groupInfo->ArenaType;
                if (!arenaType)
                    continue;

                auto arenaTypeString = Acore::StringFormatFmt("{}v{}", arenaType, arenaType);
                uint32 playersNeed = ArenaTeam::GetReqPlayersForType(arenaType);
                uint32 minLevel = std::min(bracketEntry->minLevel, (uint32)80);
                uint32 maxLevel = std::min(bracketEntry->maxLevel, (uint32)80);
                uint32 qPlayers = bgQueue.GetPlayersCountInGroupsQueue(bracket, BG_QUEUE_NORMAL_HORDE) + bgQueue.GetPlayersCountInGroupsQueue(bracket, BG_QUEUE_NORMAL_ALLIANCE);
                if (!qPlayers)
                    continue;

                /*
                    std::string, // arena type
                    uint32, // level min
                    uint32, // level max
                    uint32, // queued total
                    uint32> // players need
                */

                bool found = false;

                for (auto& [_arenaType, __, ___, _qTotal, _____] : queueArenaNonRatedList)
                {
                    if (_arenaType == arenaTypeString)
                    {
                        _qTotal = qPlayers;
                        found = true;
                    }
                }

                if (!found)
                    queueArenaNonRatedList.emplace_back(arenaTypeString, minLevel, maxLevel, qPlayers, playersNeed);
            }
        }
    };

    // Non rated arena
    for (uint8 qtype = BATTLEGROUND_QUEUE_2v2; qtype < BATTLEGROUND_QUEUE_5v5; ++qtype)
        for (uint8 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
            AddCache(BattlegroundQueueTypeId(qtype), BattlegroundBracketId(bracket));
}

void QueueListCache::UpdateBg()
{
    queueBgNormalList.clear();
    queueBgPremadeList.clear();
    queueCFBGList.clear();

    auto AddCacheToVector = [](QueueBgList* queueList, GroupQueueInfo const* groupQueueInfo, BattlegroundQueue* bgQueue, BattlegroundBracketId bracket)
    {
        Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(groupQueueInfo->BgTypeId));
        if (!bg || bg->isArena())
            return;

        PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg->GetMapId(), BattlegroundBracketId(groupQueueInfo->BracketId));
        if (!bracketEntry)
            return;

        uint32 MinPlayers = bg->GetMinPlayersPerTeam();
        uint32 MaxPlayers = MinPlayers * 2;
        uint32 minLevel = std::min(bracketEntry->minLevel, (uint32)80);
        uint32 maxLevel = std::min(bracketEntry->maxLevel, (uint32)80);

        auto playerCountInQueues = bgQueue->GetPlayersCountInGroupsQueue(bracket, BattlegroundQueueGroupTypes(groupQueueInfo->GroupType));
        if (!playerCountInQueues)
            return;

        ASSERT(queueList, "> queueList is nullptr!");

        /*
            std::string, // bg name
            uint32, // level min
            uint32, // level max
            uint32, // queued total
            uint32> // players need
        */

        bool found = false;

        for (auto& [bgName, __, ___, _qTotal, ____] : *queueList)
        {
            if (bgName == bg->GetName())
            {
                _qTotal = playerCountInQueues;
                found = true;
            }
        }

        if (!found)
            queueList->emplace_back(bg->GetName(), minLevel, maxLevel, playerCountInQueues, MaxPlayers);
    };

    auto AddCache = [this, &AddCacheToVector](Optional<bool> isPremade, BattlegroundQueueTypeId qtype, BattlegroundBracketId bracket, BattlegroundQueueGroupTypes queue1, Optional<BattlegroundQueueGroupTypes> queue2 = {})
    {
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(qtype);

        // Skip if all empty
        if (bgQueue.IsAllQueuesEmpty(bracket))
            return;

        QueueBgList* queueCacheList = nullptr;

        if (isPremade && *isPremade && queue2)
            queueCacheList = &queueBgPremadeList;
        else if (isPremade && !*isPremade && queue2)
            queueCacheList = &queueBgNormalList;
        else if (!isPremade && !queue2)
            queueCacheList = &queueCFBGList;
        else
            ABORT("> Unknown QueueBgTemplate");

        ASSERT(queueCacheList, "> queueCacheList is nullptr!");

        for (auto const& groupInfo : bgQueue.m_QueuedGroups[bracket][static_cast<uint32>(queue1)])
            AddCacheToVector(queueCacheList, groupInfo, &bgQueue, bracket);

        if (queue2)
            for (auto const& groupInfo : bgQueue.m_QueuedGroups[bracket][static_cast<uint32>(*queue2)])
                AddCacheToVector(queueCacheList, groupInfo, &bgQueue, bracket);
    };

    for (uint32 qtype = BATTLEGROUND_QUEUE_AV; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
    {
        for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
        {
            AddCache(false, BattlegroundQueueTypeId(qtype), BattlegroundBracketId(bracket), BG_QUEUE_NORMAL_ALLIANCE, BG_QUEUE_NORMAL_HORDE);
            AddCache(true, BattlegroundQueueTypeId(qtype), BattlegroundBracketId(bracket), BG_QUEUE_PREMADE_ALLIANCE, BG_QUEUE_PREMADE_HORDE);
            AddCache({}, BattlegroundQueueTypeId(qtype), BattlegroundBracketId(bracket), BG_QUEUE_CFBG);
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
            handler->PSendSysMessage("> %s (%uv%u): %u", teamName.c_str(), arenaType, arenaType, teamRating);
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
            handler->PSendSysMessage("> %u-%u %s: %u/%u", minLevel, maxLevel, bgName.c_str(), qTotal, MaxPlayers);
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
