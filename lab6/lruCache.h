#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include "hashtable.h"

typedef struct list_node
{
    char *key;
    content *cont;
    struct list_node *prev, *next;
} list_node;

typedef struct lru_cache
{
    hashtable *table;
    list_node *head, *tail;
    int capacity;
    int size;
#ifdef HASHTHREAD
    pthread_mutex_t lock;
#endif
} lru_cache;

lru_cache *createLRUCache(int capacity);
void freeLRUCache(lru_cache *cache);
content *getLRUCache(lru_cache *cache, char *key);
void putLRUCache(lru_cache *cache, char *key, content *cont);
void evictLRUCache(lru_cache *cache);

#ifdef HASHTHREAD
void lockCache(lru_cache *cache);
void unlockCache(lru_cache *cache);
#endif

#endif // LRU_CACHE_H
