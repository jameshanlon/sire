#ifndef TABLE_H
#define TABLE_H

#include <stdio.h>
#include "util.h"

typedef struct table_ *table;

table tab_New(void);
void  tab_insert(table, string key, void *value);
void *tab_lookup(table, string key);
void *tab_pop(table, string key);
void  tab_dump(table, FILE *, void (*show)(FILE *, string, void *));
int   tab_size(table);

#endif
