/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#ifndef _QUEUE_LIST_CACHE_H_
#define _QUEUE_LIST_CACHE_H_

#include "Define.h"
#include "TaskScheduler.h"
#include <tuple>
//#include <unordered_map>
#include <vector>

class ChatHandler;

class AC_GAME_API QueueListCache
{
    QueueListCache() = default;
    ~QueueListCache() = default;

    QueueListCache(QueueListCache const&) = delete;
    QueueListCache(QueueListCache&&) = delete;
    QueueListCache& operator= (QueueListCache const&) = delete;
    QueueListCache& operator= (QueueListCache&&) = delete;

public:
    static QueueListCache* instance();

    void Init(bool reload = false);
    void Update(uint32 diff);

    // Show queue status
    void ShowArenaRated(ChatHandler* handler);
    void ShowArenaNonRated(ChatHandler* handler);
    void ShowBg(ChatHandler* handler);

private:
    void UpdateArenaRated();
    void UpdateArenaNonRated();
    void UpdateBg();

    using QueueArenaRatedTemplate = std::tuple<
        std::string, // team name
        uint8, // arena type
        uint32>; // team rating

    using QueueArenaNonRatedTemplate = std::tuple<
        std::string, // arena type
        uint32, // level min
        uint32, // level max
        uint32, // queued total
        uint32>; // players need

    using QueueBgTemplate = std::tuple<
        std::string, // bg name
        uint32, // level min
        uint32, // level max
        uint32, // queued total
        uint32>; // players need

    using QueueBgList = std::vector<QueueBgTemplate>;

    // Core queue arena
    std::vector<QueueArenaRatedTemplate> queueArenaRatedList;
    std::vector<QueueArenaNonRatedTemplate> queueArenaNonRatedList;

    // Core queue bg
    QueueBgList queueBgNormalList;
    QueueBgList queueBgPremadeList;

    // Modules queue bg
    QueueBgList queueCFBGList; // mod-cfbg

    bool _isEnableSystem = false;

    TaskScheduler scheduler;
};

#define sQueueListCache QueueListCache::instance()

#endif /* _QUEUE_LIST_CACHE_H_ */
