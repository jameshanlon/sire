/*
 * A generic hash table.
 *
 * TODO: double the array size when avergae bucket length > 2.
 */

#include <stdio.h>
#include "table.h"
#include "util.h"
#include "list.h"

#define TAB_SIZE 127

typedef struct item_ *item;

/* Key-value pair with links to next binder in bucket and last binding that was
   inserted in the table */
struct item_ {
    string key;
    void *value;
};

// Table data structure
struct table_ {
    int size;
    list table[TAB_SIZE];
};

// String hashing function
static unsigned int hash(string s) {
    unsigned int hash = 5381;
    char *c = s;
    while (*c != '\0')
        hash = ((hash << 5) + hash) + (unsigned int)*c++;
    return hash % TAB_SIZE;
}

// Create a new item
static item Item(string key, void *value) {
    item p = (item) chkalloc(sizeof(*p));
    p->key = key;
    p->value = value;
    return p;
}

// Create a new empty table
table tab_New(void) {
    int i;
    table t = chkalloc(sizeof(*t));
    t->size = 0;
    for(i=0; i<TAB_SIZE; i++)
        t->table[i] = list_New();
    return t;
}

// Insert a new item in the table
void tab_insert(table t, string key, void *value) {
    int index;
    assert(t != NULL && key != NULL);
    index = hash(key);
    list_insertFirst(t->table[index], Item(key, value));
    t->size++;
}

// Retrieve a value with a key
void *tab_lookup(table t, string key) {
    assert(t != NULL && key != NULL);
    iterator it = it_begin(t->table[hash(key)]);
    while(it_hasNext(it)) {
        item p = (item) it_next(it);
        if(streq(p->key, key)) { 
            it_free(&it);
            return p->value;
        }
    }
    it_free(&it);
    return NULL;
}

// Pop the top item from a bucket
void *tab_pop(table t, string key) {
    assert(t != NULL && key != NULL);
    t->size--;
    return list_removeFirst(t->table[hash(key)]);
}

// Dump the contents of the table to text
void tab_dump(table t, FILE *out, void (*show)(FILE *, string, void *)) {
    int i;
    for(i=0; i<TAB_SIZE; i++) {
        iterator it = it_begin(t->table[i]);
        while(it_hasNext(it)) {
            item itm = (item) it_next(it);
            show(out, itm->key, itm->value);
        }
    }
}

int tab_size(table t) {
    return t->size;
}
