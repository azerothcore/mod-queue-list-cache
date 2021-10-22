/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#include "QueueListCache.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Chat.h"

using namespace Acore::ChatCommands;

class QueueListCache_Command : public CommandScript
{
public:
    QueueListCache_Command() : CommandScript("QueueListCache_Command") { }

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
    }

    static bool HandleQueueShowArenaRatedCommand(ChatHandler* handler)
    {
        sQueueListCache->ShowArenaRated(handler);
        return true;
    }

    static bool HandleQueueShowArenaNormalCommand(ChatHandler* handler)
    {
        sQueueListCache->ShowArenaNonRated(handler);
        return true;
    }

    static bool HandleQueueShowBgCommand(ChatHandler* handler)
    {
        sQueueListCache->ShowBg(handler);
        return true;
    }
};

class QueueListCache_World : public WorldScript
{
public:
    QueueListCache_World() : WorldScript("QueueListCache_World") { }

    void OnBeforeConfigLoad(bool reload) override
    {
        sQueueListCache->Init(reload);
    }

    void OnStartup() override
    {
        LOG_INFO("server.loading", "Loading Queue cache...");
        sQueueListCache->Init(false);
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
