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

void freeLRUCache(lru_cache *cache)
{
    if (!cache)
        return;

    // Free the linked list
    list_node *current = cache->head;
    while (current)
    {
        list_node *temp = current;
        current = current->next;
        free(temp->key);
        free(temp->cont); // Assuming content needs to be freed
        free(temp);
    }

    // Free the hash table
    freeHashTable(cache->table);

#ifdef HASHTHREAD
    pthread_mutex_destroy(&cache->lock);
#endif

    free(cache);
}
