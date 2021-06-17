/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#include "QueueListCache.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Chat.h"

// using namespace Acore::ChatCommands;

class QueueListCache_Command : public CommandScript
{
public:
    QueueListCache_Command() : CommandScript("QueueListCache_Command") { }

    /* New api chat commands
    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable queueShowArenaCommandTable = // .queue show arena
        {
            { "rated",  HandleQueueShowArenaRatedCommand,   SEC_PLAYER, Console::Yes },
            { "normal", HandleQueueShowArenaNormalCommand,  SEC_PLAYER, Console::Yes }
        };

        static ChatCommandTable queueShowCommandTable = // .queue show
        {
            { "bg",     HandleQueueShowBgCommand,           SEC_PLAYER, Console::Yes },
            { "arena", queueShowArenaCommandTable }
        };

        static ChatCommandTable queueCommandTable = // .queue
        {
            { "show", queueShowCommandTable }
        };

        static ChatCommandTable commandTable =
        {
            { "queue", queueCommandTable }
        };

        return commandTable;
    }*/

    // Old api, need replace after start support new chat command api
    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> queueShowArenaCommandTable // .queue show arena
        {
            { "rated",   SEC_PLAYER,  true,  &HandleQueueShowArenaRatedCommand,      "" },
            { "normal",  SEC_PLAYER,  true,  &HandleQueueShowArenaNormalCommand,     "" },
        };

        static std::vector<ChatCommand> queueShowCommandTable = // .queue show
        {
            { "arena",  SEC_PLAYER, true, nullptr, "", queueShowArenaCommandTable },
            { "bg",     SEC_PLAYER,         false,  &HandleQueueShowBgCommand, "" }
        };

        static std::vector<ChatCommand> queueCommandTable = // .queue
        {
            { "show",   SEC_PLAYER, true, nullptr, "", queueShowCommandTable }
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "queue",  SEC_PLAYER, true, nullptr, "", queueCommandTable }
        };

        return commandTable;
    }

    static bool HandleQueueShowArenaRatedCommand(ChatHandler* handler, char const* /*args*/)
    {
        sQueueListCache->ShowArenaRated(handler);
        return true;
    }

    static bool HandleQueueShowArenaNormalCommand(ChatHandler* handler, char const* /*args*/)
    {
        sQueueListCache->ShowArenaNonRated(handler);
        return true;
    }

    static bool HandleQueueShowBgCommand(ChatHandler* handler, char const* /*args*/)
    {
        sQueueListCache->ShowBg(handler);
        return true;
    }
};


class QueueListCache_World : public WorldScript
{
public:
    QueueListCache_World() : WorldScript("QueueListCache_World") { }

    void OnStartup() override
    {
        LOG_INFO("server.loading", "Loading Queue cache...");
        sQueueListCache->Init();
    }

    void OnUpdate(uint32 diff) override
    {
        sQueueListCache->Update(diff);
    }
};

// Group all custom scripts
void AddSC_QueueListCache()
{
    new QueueListCache_Command();
    new QueueListCache_World();
}
