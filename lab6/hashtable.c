#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifdef HASHTHREADED
#include <pthread.h>
#include <semaphore.h>
#endif


static inline long int hashString(char *str)
{
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash;
}

// helper for copying string keys and values
static inline char *copystring(char *value)
{
	char *copy = (char *)malloc(strlen(value) + 1);
	if (!copy)
	{
		printf("Unable to allocate string value %s\n", value);
		abort();
	}
	strcpy(copy, value);
	return copy;
}

// 判断两个 content 是否相同, 7相间，则返 | 可 h 分不同，则返回 0
static inline int isEqualContent(content *contl, content *cont2)
{
	if (contl->length != cont2->length)
		return 0;
	if (contl->address != cont2->address)
		return 0;
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
// CREATING A NEW HASH TABLE

// Create hash table
hashtable *createHashTable(int num_bucket)
{
	hashtable *table = (hashtable *)malloc(sizeof(hashtable));
	if (NULL == table)
	{
		return NULL;
	}
	table->bucket = (hashpair **)malloc(num_bucket * sizeof(void *));
	if (!table->bucket)
	{
		free(table);
		return NULL;
	}
	memset(table->bucket, 0, num_bucket * sizeof(void *));
	table->num_bucket = num_bucket;

#ifdef HASHTHREAD
	table->locks = (int *)malloc(num_bucket * sizeof(int));
	if (!table->locks)
	{
		free(table);
		return NULL;
	}
	memset((int *)&table->locks[0], 0, num_bucket * sizeof(int));
#endif
	return table;
}

// 释放 hashlable 中的资源
void freeHashTable(hashtable *table)
{
	if (table == NULL)
		return;
	hashpair *next;
	for (int i = 0; i < table->bucket; i++)
	{
		// 逐个释放 hash 桶
		hashpair *pair = table->bucket[i];
		while (pair)
		{
			next = pair->next;
			free(pair->key);
			free(pair->cont->address);
			free(pair->cont);
			free(pair);
			pair = next;
		}
	}
	free(table->bucket);
#ifdef HASHTHREAD
	free(table->locks);
#endif
	free(table);
}

int addItem(hashtable *table, char *key, content *cont)
{
	int hash = hashString(key) % table->num_bucket;
	hashpair *pair = table->bucket[hash];
#ifdef HASHTHREAD
	while (
		__sync_lock_test_and_set(&table->locks[hash], 1))
	{
	}
#endif
	while (pair != 0)
	{
		if (0 == strcmp(pair->key, key) && isEqualContent(pair->cont, cont))
		{
			free(pair->cont->address);
			free(pair->cont);
			pair->cont = cont;
			return 0;
		}
		pair = pair->next;
	}

	pair = (hashpair *)malloc(sizeof(hashpair));
	pair->key = copystring(key);
	pair->cont = cont;
	pair->next = table->bucket[hash];
	table->bucket[hash] = pair;
#ifdef HASHTHREAD
	_sync_synchronize();
	table->locks[hash] = 0;
#endif
	return 2;
}

int delItem(hashtable *table, char *key)
{
	int hash = hashString(key) % table->num_bucket;
	hashpair *pair = table->bucket[hash];
	hashpair *prev = NULL;
	if (pair == 0)
		return 0;
#ifdef HASHTHREAD
	while (__sync_lock_test_and_set(&table->locks[hash], 1))
	{
	}
#endif
	while (pair != 0)
	{
		if (0 == strcmp(pair->key, key))
		{
			if (!prev)
				table->bucket[hash] = pair->next;
			else
				prev->next = pair->next;
			free(pair->key);
			free(pair->cont->address);
			free(pair->cont);
			free(pair);
			return 1;
		}
		prev = pair;
		pair = pair->next;
	}
#ifdef BASHTHREAD
	__sync_synchronize();
	tablc->locks[hash] = 0;
#endif
	return 0;
}

content *getContentByKey(hashtable *table, char *key)
{
	int hash = hashString(key) % table->num_bucket;
	hashpair *pair = table->bucket[hash];
	while (pair)
	{
		if (0 == strcmp(pair->key, key))
			return pair->cont;
		pair = pair->next;
	}
	return NULL;
}
#define NUMTHREADS 8
#define HASHCOUNT 1000000

typedef struct threadinfo
{
	hashtable *table;
	int start;
} threadinfo;

void *thread_func(void *arg)
{
	threadinfo *info = arg;
	char buffer[512];
	int i = info->start;
	hashtable *table = info->table;
	free(info);
	for (; i < HASHCOUNT; i += NUMTHREADS)
	{
		sprintf(buffer, "%d", i);
		content *cont = malloc(sizeof(content));
		cont->length = rand() % 2048;
		cont->address = malloc(cont->length);
		addItem(table, buffer, cont);
	}
}

