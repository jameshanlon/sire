#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include "util.h"

/* A generic doubly linked list implementation. List elements are accessed using
 * an internal iterator.
 */

typedef struct list_ *list;

list      list_New(void);
list      list_Copy(list);
void      list_add(list, void *);
void      list_appendList(list, list);
void      list_insertFirst(list, void *);
void      list_insertLast(list, void *);
void     *list_removeFirst(list);
void     *list_removeLast(list);
void     *list_head(list);
void     *list_tail(list);
bool      list_contains(list, void *, bool(*equal)(void *, void *));
list      list_get(list, void *, bool(*equal)(void *, void *));
void     *list_getFirst(list, void *, bool(*equal)(void *, void *));
void     *list_remove(list, void *, bool(*equal)(void *, void *));
void      list_clear(list);
void      list_delete(list);
void      list_deepDelete(list, void(*deleteItem)(void *));
int       list_size(list);
bool      list_empty(list);
void      list_dump(list, FILE *out, string(*toString)(void *));

typedef struct iterator_ *iterator;

iterator  it_begin(list);
iterator  it_end(list);
iterator  it_copy(iterator);
bool      it_hasNext(iterator);
bool      it_hasPrev(iterator);
void     *it_peekNext(iterator);
void     *it_peekPrev(iterator);
void     *it_next(iterator);
void     *it_prev(iterator);
void     *it_curr(iterator);
void      it_replace(iterator, void *);
void      it_insertBefore(iterator, void *);
void      it_insertAfter(iterator, void *);
void     *it_remove(iterator);
void      it_free(iterator *);

#endif
