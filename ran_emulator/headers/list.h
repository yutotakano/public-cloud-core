#ifndef __list__
#define __list__

typedef struct _List List;

List * initList(int (*cmp)(void*, void*));
int addElem(List * l, void * elem);
void * peekElem(List * l, void * key);
void * getElem(List * l, void * key);
void destroyList(List * l, void (*destroy)(void *));

#endif