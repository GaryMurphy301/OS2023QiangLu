#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdlib.h>

#ifdef HASHTHREADED
#include <pthread.h>
#include <semaphore.h>
#endif

// Data structures
typedef struct content {
    int length;
    void *address;
} content;

typedef struct hashpair {
    char *key;
    content *cont;
    struct hashpair *next;
} hashpair;

typedef struct hashtable {
    hashpair **bucket;
    int num_bucket;
#ifdef HASHTHREAD
    volatile int *locks;
#endif
} hashtable;

// Function prototypes
hashtable *createHashTable(int num_bucket);
void freeHashTable(hashtable *table);
int addItem(hashtable *table, char *key, content *cont);
int delItem(hashtable *table, char *key);
content *getContentByKey(hashtable *table, char *key);

#ifdef HASHTHREAD
void *thread_func(void *arg);
#endif

#endif // HASHTABLE_H

