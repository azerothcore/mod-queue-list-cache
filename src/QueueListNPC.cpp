#include "QueueListNPC.h"
#include "QueueListCache.h"

queue_list_npc::queue_list_npc() : CreatureScript("queue_list_npc") { }

bool queue_list_npc::OnGossipHello(Player* player, Creature* creature)
{
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_arena_2v2_7:25|t Show Rated Arena queues", GOSSIP_SENDER_MAIN, 1);
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_arena_2v2_2:25|t Show Skirmish Arena queues", GOSSIP_SENDER_MAIN, 2);
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_bg_killxenemies_generalsroom:25|t Show BG queues", GOSSIP_SENDER_MAIN, 3);

    player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
    return true;
}

bool queue_list_npc::OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
{
    player->PlayerTalkClass->ClearMenus();

    ChatHandler handler(player->GetSession());

    switch (uiAction)
    {
        case 1:
            sQueueListCache->ShowArenaRated(&handler);
            break;
        case 2:
            sQueueListCache->ShowArenaNonRated(&handler);
            break;
        case 3:
            sQueueListCache->ShowBg(&handler);
            break;
    }

    CloseGossipMenuFor(player);

    return true;
}
