#include "lruCache.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void testLRUCache()
{
    // Create a cache with a small capacity
    int capacity = 3;
    LRUCache *cache = createCache(capacity);

    // Add items to the cache
    addToCache(cache, "key1", "value1");
    addToCache(cache, "key2", "value2");
    addToCache(cache, "key3", "value3");

    // Check that all items are in the cache
    assert(strcmp(getFromCache(cache, "key1"), "value1") == 0);
    assert(strcmp(getFromCache(cache, "key2"), "value2") == 0);
    assert(strcmp(getFromCache(cache, "key3"), "value3") == 0);

    // Add another item, causing the first one to be evicted
    addToCache(cache, "key4", "value4");

    // Check that key1 is no longer in the cache
    assert(getFromCache(cache, "key1") == NULL);

    // Check that other items are still there
    assert(strcmp(getFromCache(cache, "key2"), "value2") == 0);
    assert(strcmp(getFromCache(cache, "key3"), "value3") == 0);
    assert(strcmp(getFromCache(cache, "key4"), "value4") == 0);

    // Remove an item
    removeFromCache(cache, "key2");

    // Check that key2 is no longer in the cache
    assert(getFromCache(cache, "key2") == NULL);

    // Clean up
    freeCache(cache);

    printf("All tests passed!\n");
}

int main()
{
    testLRUCache();
    return 0;
}
