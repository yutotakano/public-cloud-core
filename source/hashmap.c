#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "hashmap.h"
#include "jhash.h"

#define OK 0
#define ERROR -1

struct _HashMap
{
	uint32_t initval;
	uint32_t hashmap_size;
	uint32_t (*hash)(void*);
	void ** table;
};

HashMap * init_hashmap(uint32_t size, uint32_t (*hash)(void*))
{
	HashMap * hm;
	hm = (HashMap*)malloc(sizeof(HashMap));
	if(hm == NULL)
		return NULL;
	/* Store values */
	hm->hash = hash;
	hm->hashmap_size = size;
	/* Generate initval */
	srand(time(0));
	hm->initval = (uint32_t)rand();
	/* Allocate the hash table */
	/* NOTE: Using calloc to initialize every entry to NULL/0 */
	hm->table = (void**)calloc(size, sizeof(void*));
	if(hm->table == NULL) {
		free(hm);
		return NULL;
	}
	return hm;
}

void free_hashmap(HashMap * hm, void (*destroy)(void *))
{
	int i;

	for(i = 0; i < hm->hashmap_size; i++) {
		/* Delete every item in the list */
		destroy(hm->table[i]);
	}
	free(hm->table);
	free(hm);
}

int hashmap_add(HashMap * hm, uint8_t * data, int size)
{
	uint32_t index, hash;
	void * data_copy;

	/* Creating a copy of data */
	data_copy = malloc(size);
	if(data_copy == NULL)
		return ERROR;
	memcpy((uint8_t *)data_copy, data, size);


	/* Get the entry index in the table */
	hash = hm->hash(data);
	index = jhash(hash, hm->initval) % hm->hashmap_size;

	/* Insert new node in table[index] */
	hm->table[index] = data_copy;

	return OK;
}

void * hashmap_get(HashMap * hm, uint32_t hash)
{
	return hm->table[jhash(hash, hm->initval) % hm->hashmap_size];
}

