#ifndef SET_H
#define SET_H

#include "list.h"

typedef struct set_ *set;

set set_New(bool (*equal)(void *, void *));
void   set_add(set, void *);
set    set_copy(set);
void   set_union(set, set);
void   set_append(set, set);
void   set_minus(set, set);
void   set_replace(set, set);
bool   set_contains(set, void *, bool (*equal)(void *, void *));
list   set_get(set, void *, bool (*equal)(void *, void *));
bool   set_equal(set, set);
int    set_size(set);
list   set_elements(set);
void   set_clear(set);
void   set_delete(set);
string set_string(set, string (*show)(void *));

#endif
