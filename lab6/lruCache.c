#include "lruCache.h"
lru_cache *createLRUCache(int capacity)
{
    lru_cache *cache = malloc(sizeof(lru_cache));
    if (!cache)
        return NULL;

    cache->table = createHashTable(capacity); // Use your hashtable implementation
    if (!cache->table)
    {
        free(cache);
        return NULL;
    }

    cache->head = NULL;
    cache->tail = NULL;
    cache->capacity = capacity;
    cache->size = 0;

#ifdef HASHTHREAD
    pthread_mutex_init(&cache->lock, NULL);
#endif

    return cache;
}
