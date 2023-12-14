#include "lruCache.h"
#include <stdlib.h>
#include <string.h>

// Helper function to create a new Cache Item
static CacheItem *newCacheItem(char *key, char *value)
{
    CacheItem *item = (CacheItem *)malloc(sizeof(CacheItem));
    item->key = strdup(key);
    item->value = strdup(value);
    item->prev = item->next = NULL;
    return item;
}

// Function to create a new cache
LRUCache *createCache(int capacity)
{
    LRUCache *cache = (LRUCache *)malloc(sizeof(LRUCache));
    cache->capacity = capacity;
    cache->size = 0;
    cache->head = cache->tail = NULL;
    // Additional initialization if necessary
    return cache;
}

// Move the item to the front of the cache (most recently used)
static void moveToHead(LRUCache *cache, CacheItem *item)
{
    if (item->prev)
        item->prev->next = item->next;
    if (item->next)
        item->next->prev = item->prev;
    if (item == cache->tail && item->prev)
        cache->tail = item->prev;

    item->next = cache->head;
    item->prev = NULL;
    if (cache->head)
        cache->head->prev = item;
    cache->head = item;
    if (!cache->tail)
        cache->tail = cache->head;
}

// Function to access an item from the cache
char *getFromCache(LRUCache *cache, char *key)
{
    CacheItem *current = cache->head;
    while (current)
    {
        if (strcmp(current->key, key) == 0)
        {
            moveToHead(cache, current);
            return current->value;
        }
        current = current->next;
    }
    return NULL; // Not found
}

// Function to remove the least recently used item from the cache
static void removeLRUItem(LRUCache *cache)
{
    if (!cache->tail)
        return;

    CacheItem *lru = cache->tail;
    if (lru->prev)
        lru->prev->next = NULL;
    else
        cache->head = NULL;

    cache->tail = lru->prev;
    free(lru->key);
    free(lru->value);
    free(lru);
    cache->size--;
}

// Function to add an item to the cache
void addToCache(LRUCache *cache, char *key, char *value)
{
    // First, check if the item is already in the cache
    char *val = getFromCache(cache, key);
    if (val)
    {
        // Update the value and move it to the head
        free(val);
        val = strdup(value);
        return;
    }

    // If the cache is full, remove the least recently used item
    if (cache->size == cache->capacity)
    {
        removeLRUItem(cache);
    }

    // Create a new cache item and insert it at the head
    CacheItem *item = newCacheItem(key, value);
    moveToHead(cache, item);
    cache->size++;
}

// Function to remove an item from the cache
void removeFromCache(LRUCache *cache, char *key)
{
    CacheItem *current = cache->head;
    while (current)
    {
        if (strcmp(current->key, key) == 0)
        {
            if (current->prev)
                current->prev->next = current->next;
            if (current->next)
                current->next->prev = current->prev;

            if (current == cache->head)
                cache->head = current->next;
            if (current == cache->tail)
                cache->tail = current->prev;

            free(current->key);
            free(current->value);
            free(current);
            cache->size--;
            return;
        }
        current = current->next;
    }
}

// Function to clear and free the cache
void freeCache(LRUCache *cache)
{
    while (cache->head)
    {
        removeFromCache(cache, cache->head->key);
    }
    free(cache);
}
