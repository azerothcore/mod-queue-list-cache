/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#ifndef _QUEUE_LIST_CACHE_H_
#define _QUEUE_LIST_CACHE_H_

#include "Define.h"

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
};

#define sQueueListCache QueueListCache::instance()

#endif /* _QUEUE_LIST_CACHE_H_ */
