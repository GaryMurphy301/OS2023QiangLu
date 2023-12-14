#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdbool.h>

// Define the structure for cache items
typedef struct CacheItem
{
    char *key;
    char *value;
    struct CacheItem *prev, *next;
} CacheItem;

// Define the structure for the LRU cache
typedef struct LRUCache
{
    int capacity;
    int size;
    CacheItem *head, *tail;
    // Additional fields like a hash table for fast access may be included
} LRUCache;

// Function to create a new cache
LRUCache *createCache(int capacity);

// Function to access an item from the cache
char *getFromCache(LRUCache *cache, char *key);

// Function to add an item to the cache
void addToCache(LRUCache *cache, char *key, char *value);

// Function to remove an item from the cache
void removeFromCache(LRUCache *cache, char *key);

// Function to clear and free the cache
void freeCache(LRUCache *cache);

#endif // LRU_CACHE_H
