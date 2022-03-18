#include <stdlib.h>
#include <string.h>
#include "set.h"

#include "temp.h"

// The set object data structure 
struct set_ {
    bool(*equal)(void *, void *);
    list elements;
};

// Constructor
set set_New(bool (*equal)(void *, void *)) {
    set s = (set) chkalloc(sizeof(*s));
    s->equal = equal;
    s->elements = list_New();
    return s;
}

// Add a new element only if it wasn't already a member
void set_add(set s, void *element) {
    if(!list_contains(s->elements, element, s->equal))
        list_add(s->elements, element);
}

set set_copy(set s) {
    set new = set_New(s->equal);
    list_appendList(new->elements, s->elements);
    return new;
}

// a = a union b
void set_union(set a, set b) {
    iterator it = it_begin(b->elements);
    while(it_hasNext(it)) {
        void *element = it_next(it);
        if(!list_contains(a->elements, element, a->equal))
            list_add(a->elements, element);
    }
    it_free(&it);
}

// Union regardless of equality function
// NOTE: bit of a hack to get around difference in union required by liveness
// and register assignment phases of regalloc
void set_append(set a, set b) {
    list_appendList(a->elements, b->elements);
}

// a = a - b
void set_minus(set a, set b) {
    iterator it = it_begin(b->elements);
    while(it_hasNext(it)) {
        void *element = it_next(it);
        list_remove(a->elements, element, a->equal);
    }
    it_free(&it);
}

// a = b
void set_replace(set a, set b) {
    list_clear(a->elements);
    list_appendList(a->elements, b->elements);
}

// a == b
bool set_equal(set a, set b) {
    if(set_size(a) != set_size(b))
        return false;
    iterator it = it_begin(a->elements);
    while(it_hasNext(it)) {
        if(!list_contains(b->elements, it_next(it), a->equal)) {
            it_free(&it);
            return false;
        }
    }
    it_free(&it);
    return true;
}

// Check if the set contains a value given by the function equal
bool set_contains(set s, void *value, bool (*eq)(void *, void *)) {
    return list_contains(s->elements, value, eq);
}

list set_get(set s, void *value, bool (*eq)(void *, void *)) {
    return list_get(s->elements, value, eq);
}

int set_size(set s) {
    return list_size(s->elements);
}

void set_clear(set s) {
    list_clear(s->elements);
}

list set_elements(set s) {
    return s->elements;
}

void set_delete(set s) {
    if(s != NULL) {
        list_clear(s->elements);
        free(s);
    }
}

// Return a string represenataion of a set of temporaries
string set_string(set s, string (*show)(void *)) {
    char str[100] = "";
    iterator it = it_begin(s->elements);
    while(it_hasNext(it)) {
        void *elem = it_next(it);
        strcat(str, StringFmt("%s%s", show(elem), it_hasNext(it)?", ":""));
    }
    it_free(&it);
    return String(str);
}

