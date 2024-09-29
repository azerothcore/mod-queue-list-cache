#include "QueueListNPC.h"
#include "QueueListCache.h"

QueueListCache_Npc::QueueListCache_Npc() : CreatureScript("queue_list_npc") { }

bool QueueListCache_Npc::OnGossipHello(Player* player, Creature* creature)
{
    if (!player || !creature)
        return true;

    if (sConfigMgr->GetOption<bool>("QLC.Enable", true) == false)
    {
        ChatHandler(player->GetSession()).SendSysMessage("NPC disabled!");
        return true;
    }

    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_arena_2v2_7:30:30:-20:0|t Show Rated Arena queues", GOSSIP_SENDER_MAIN, SHOW_RATED_ARENA_QUEUES);
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_arena_2v2_2:30:30:-20:0|t Show Skirmish Arena queues", GOSSIP_SENDER_MAIN, SHOW_SKIRMISH_ARENA_QUEUES);
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\icons\\Achievement_bg_killxenemies_generalsroom:30:30:-20:0|t Show BG queues", GOSSIP_SENDER_MAIN, SHOW_BATTLEGROUND_QUEUES);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|TInterface/ICONS/inv_misc_questionmark:30:30:-20:0|t Help", GOSSIP_SENDER_MAIN, ACTION_HELP);

    player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
    return true;
}

bool QueueListCache_Npc::OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
{
    if (!player || !creature)
        return true;

    player->PlayerTalkClass->ClearMenus();

    ChatHandler handler(player->GetSession());

    switch (uiAction)
    {
        case SHOW_MAIN_MENU:
            OnGossipHello(player, creature);
            break;
        case SHOW_RATED_ARENA_QUEUES:
            sQueueListCache->ShowArenaRated(&handler);
            CloseGossipMenuFor(player);
            break;
        case SHOW_SKIRMISH_ARENA_QUEUES:
            sQueueListCache->ShowArenaNonRated(&handler);
            CloseGossipMenuFor(player);
            break;
        case SHOW_BATTLEGROUND_QUEUES:
            sQueueListCache->ShowBg(&handler);
            CloseGossipMenuFor(player);
            break;
        case ACTION_HELP:
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "<- Back", GOSSIP_SENDER_MAIN, SHOW_MAIN_MENU);
            SendGossipMenuFor(player, NPC_TEXT_ENTRY_QUEUE_LIST, creature->GetGUID());
            break;
        default:
            break;
    }

    return true;
}
