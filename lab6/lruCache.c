#include "lruCache.h"

// Helper function to create a new list node
static list_node *createListNode(char *key, content *cont)
{
    list_node *node = (list_node *)malloc(sizeof(list_node));
    if (node == NULL)
    {
        return NULL;
    }
    node->key = strdup(key); // Copy the key
    node->cont = cont;
    node->prev = node->next = NULL;
    return node;
}

// Move a node to the front of the list (most recently used)
static void moveToFront(lru_cache *cache, list_node *node)
{
    if (cache->head == node)
    {
        return; // Node is already at the front
    }

    // Detach node from its current position
    if (node->prev)
    {
        node->prev->next = node->next;
    }
    if (node->next)
    {
        node->next->prev = node->prev;
    }

    // Place node at the front
    node->next = cache->head;
    node->prev = NULL;
    if (cache->head)
    {
        cache->head->prev = node;
    }
    cache->head = node;

    if (cache->tail == NULL)
    {
        cache->tail = node; // First node in the list
    }
}

// Remove a node from the list
static void removeNode(lru_cache *cache, list_node *node)
{
    if (!node)
        return;

    if (node->prev)
    {
        node->prev->next = node->next;
    }
    else
    {
        cache->head = node->next; // Node is head
    }

    if (node->next)
    {
        node->next->prev = node->prev;
    }
    else
    {
        cache->tail = node->prev; // Node is tail
    }

    free(node->key);
    free(node);
}

// Evict the least recently used item (from the tail)
static void evictIfNeeded(lru_cache *cache)
{
    if (cache->size >= cache->capacity)
    {
        // Remove the tail node
        list_node *oldTail = cache->tail;
        removeNode(cache, oldTail);
        delItem(cache->table, oldTail->key); // Remove from hashtable
        free(oldTail);
        cache->size--;
    }
}

// Create a new LRU cache
lru_cache *createLRUCache(int capacity)
{
    lru_cache *cache = (lru_cache *)malloc(sizeof(lru_cache));
    if (cache == NULL)
    {
        return NULL;
    }

    cache->table = createHashTable(capacity); // Assuming a 1:1 ratio of capacity to buckets
    cache->capacity = capacity;
    cache->size = 0;
    cache->head = cache->tail = NULL;

#ifdef HASHTHREAD
    pthread_mutex_init(&cache->lock, NULL);
#endif

    return cache;
}

// Free the LRU cache
void freeLRUCache(lru_cache *cache)
{
    if (!cache)
        return;

    // Free all list nodes
    list_node *current = cache->head;
    while (current)
    {
        list_node *temp = current;
        current = current->next;
        free(temp->key);
        free(temp);
    }

    // Free the hashtable
    freeHashTable(cache->table);

#ifdef HASHTHREAD
    pthread_mutex_destroy(&cache->lock);
#endif

    free(cache);
}

// Retrieve an item from the cache
content *getLRUCache(lru_cache *cache, char *key)
{
#ifdef HASHTHREAD
    pthread_mutex_lock(&cache->lock);
#endif

    content *cont = getContentByKey(cache->table, key);
    if (cont)
    {
        // Move the corresponding node to the front
        // Assuming that the hash table stores pointers to the nodes
        list_node *node = (list_node *)cont->address;
        moveToFront(cache, node);
    }

#ifdef HASHTHREAD
    pthread_mutex_unlock(&cache->lock);
#endif

    return cont;
}

// Insert an item into the cache
void putLRUCache(lru_cache *cache, char *key, content *cont)
{
#ifdef HASHTHREAD
    pthread_mutex_lock(&cache->lock);
#endif

    content *existing = getContentByKey(cache->table, key);
    if (existing)
    {
        // Update the content and move to front
        existing->address = cont->address;
        existing->length = cont->length;
        list_node *node = (list_node *)existing->address;
        moveToFront(cache, node);
    }
    else
    {
        // Create a new node and insert
        list_node *newNode = createListNode(key, cont);
        moveToFront(cache, newNode);
        addItem(cache->table, key, (content *)newNode); // Store node in hashtable
        cache->size++;

        evictIfNeeded(cache); // Evict if needed
    }

#ifdef HASHTHREAD
    pthread_mutex_unlock(&cache->lock);
#endif
}

#ifdef HASHTHREAD
// Lock the cache
void lockCache(lru_cache *cache)
{
    pthread_mutex_lock(&cache->lock);
}

// Unlock the cache
void unlockCache(lru_cache *cache)
{
    pthread_mutex_unlock(&cache->lock);
}
#endif
