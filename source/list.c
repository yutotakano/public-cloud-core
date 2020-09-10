#include <stdlib.h>
#include "list.h"

typedef struct _Node Node;
struct _Node
{
	void * data;
	Node * prev;
	Node * next;

};

struct _List
{
	Node * head;
	Node * tail;
	int (*cmp)(void*, void*);
};

List * initList(int (*cmp)(void*, void*))
{
	List * l;
	l = malloc(sizeof(List));
	if(l == NULL)
		return NULL;
	l->head = NULL;
	l->tail = NULL;
	l->cmp = cmp;
	return l;
}

int addElem(List * l, void * elem)
{
	Node * new_node = malloc(sizeof(Node));
	if(new_node == NULL)
		return 1;
	new_node->data = elem;
	new_node->next = NULL;
	new_node->prev = l->tail;
	if(l->head == NULL) /* First element case */
		l->head = new_node;
	else
		(l->tail)->next = new_node;
	l->tail = new_node;
	return 0;
}

void * peekElem(List * l, void * key)
{
	Node * iter = l->head;

	while(iter != NULL)
	{
		if(l->cmp(iter->data, key))
			return iter->data;
		iter = iter->next;
	}
	return NULL;
}

void * getElem(List * l, void * key)
{
	Node * iter = l->head;

	while(iter)
	{
		if(l->cmp(iter->data, key))
		{
			if(iter->prev == NULL && iter->next == NULL)
			{
				l->head = NULL;
				l->tail = NULL;
			}
			else if(iter->prev == NULL)
			{
				(iter->next)->prev = NULL;
				l->head = iter->next;
			}
			else if(iter->next == NULL)
			{
				(iter->prev)->next = NULL;
				l->tail = iter->prev;
			}
			else
			{
				(iter->prev)->next = iter->next;
				(iter->next)->prev = iter->prev;
			}
			free(iter);

			return iter->data;
		}
		iter = iter->next;
	}
	return NULL;
}

void destroyList(List * l, void (*destroy)(void *))
{
	Node * iter = l->head;
	Node * iter_aux;
	while(iter)
	{
		iter_aux = iter->next;
		destroy(iter->data);
		free(iter);
		iter = iter_aux;
	}
	free(l);
}

