#ifndef QUEUE_LIST_NPC_H
#define QUEUE_LIST_NPC_H

#include "Chat.h"
#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

enum Gossips : uint8
{
    SHOW_RATED_ARENA_QUEUES = 1,
    SHOW_SKIRMISH_ARENA_QUEUES = 2,
    SHOW_BATTLEGROUND_QUEUES = 3
};

class QueueListCache_Npc : public CreatureScript
{
public:
    QueueListCache_Npc();

    bool OnGossipHello(Player* player, Creature* creature) override;
    bool OnGossipSelect(Player* player, Creature* creature, uint32 uiSender, uint32 uiAction) override;
};

#endif
