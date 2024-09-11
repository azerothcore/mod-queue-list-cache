#ifndef QUEUE_LIST_NPC_H
#define QUEUE_LIST_NPC_H

#include "Chat.h"
#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

class queue_list_npc : public CreatureScript
{
public:
    queue_list_npc();

    bool OnGossipHello(Player* player, Creature* creature) override;
    bool OnGossipSelect(Player* player, Creature* creature, uint32 uiSender, uint32 uiAction) override;
};

#endif
