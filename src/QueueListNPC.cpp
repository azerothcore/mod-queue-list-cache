#include "QueueListNPC.h"
#include "QueueListCache.h"

enum Gossips : uint8
{
    Show_RatedArena_Queues = 1,
    Show_SkirmishArena_Queues = 2,
    Show_Battleground_Queues = 3
};

queue_list_npc::queue_list_npc() : CreatureScript("queue_list_npc") { }

bool queue_list_npc::OnGossipHello(Player* player, Creature* creature)
{
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_arena_2v2_7:25|t Show Rated Arena queues", GOSSIP_SENDER_MAIN, Show_RatedArena_Queues);
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_arena_2v2_2:25|t Show Skirmish Arena queues", GOSSIP_SENDER_MAIN, Show_SkirmishArena_Queues);
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_bg_killxenemies_generalsroom:25|t Show BG queues", GOSSIP_SENDER_MAIN, Show_Battleground_Queues);

    player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
    return true;
}

bool queue_list_npc::OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
{
    player->PlayerTalkClass->ClearMenus();

    ChatHandler handler(player->GetSession());

    switch (uiAction)
    {
        case Show_RatedArena_Queues:
            sQueueListCache->ShowArenaRated(&handler);
            break;
        case Show_SkirmishArena_Queues:
            sQueueListCache->ShowArenaNonRated(&handler);
            break;
        case Show_Battleground_Queues:
            sQueueListCache->ShowBg(&handler);
            break;
    }

    CloseGossipMenuFor(player);

    return true;
}
