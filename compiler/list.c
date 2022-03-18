#include <stdlib.h>
#include "list.h"
#include "util.h"
#include "error.h"

/*
TODO: 
    - modifications to list should invalidate iterator
    - can only remove a single element per call to next()
    - more error checking
    - Catch case when try to use an iterator after it has been deleted: rethink
    deletion of iterators

    0: head (first inserted element)
    1
    .
    .
    n: tail (last inserted element)
    
    next: i --> i+1
*/

typedef struct item_ *item;

struct list_ {
    int size;
    int activeIts;
    item head;
    item tail;
    item it;
};

struct item_ {
    int id;
    void *value;
    item prev;
    item next;
};

struct iterator_ {
    bool forward;
    list l;
    item curr;
    item next;
};

// List constructor
list list_New(void) {
    list l = (list) chkalloc(sizeof(*l));
    l->size = 0;
    l->activeIts = 0;
    l->head = NULL;
    l->tail = NULL;
    l->it = NULL;
    return l;
}

// Copy a list
list list_Copy(list l) {
    list new = list_New();
    iterator it = it_begin(l);
    while(it_hasNext(it))
        list_add(new, it_next(it));
    it_free(&it);
    return new;
}

// Node constructor
item Node(void *value, item prev, item next) {
    item new = (item) chkalloc(sizeof(*new));
    new->value = value;
    new->prev = prev;
    new->next = next;
    return new;
}

// Add an item to the head of the list, an alias for insertLast
void list_add(list l, void *item) {
    list_insertLast(l, item);
}

// Insert at the head of the list
void list_insertFirst(list l, void *value) {
    item old = l->head;
    l->head = Node(value, NULL, old);
    if(old != NULL) old->prev = l->head;
    l->size++;
}

// Insert at the tail of the list
void list_insertLast(list l, void *value) {
    item old = l->tail;
    l->tail = Node(value, old, NULL);
    if(l->head == NULL) l->head = l->tail;
    if(old != NULL) old->next = l->tail;
    l->size++;
}

// Add the contents of one list to another
void list_appendList(list dst, list src) {
    //printf("appending list of size %d to list of size %d\n", src->size, dst->size);
    iterator it = it_begin(src);
    while(it_hasNext(it))
        list_insertLast(dst, it_next(it));
    it_free(&it);
}

// Remove the first entry
void *list_removeFirst(list l) {
    if(l->head == NULL) 
        return NULL;
    item removed = l->head;
    l->head = removed->next;
    if(removed->next != NULL)
        l->head->prev = NULL;
    void *value = removed->value;
    free(removed);
    l->size--;
    assert(l->size >= 0);
    return value;
}

// Remove the last entry
void *list_removeLast(list l) {
    if(l->tail == NULL) 
        return NULL;
    item removed = l->tail;
    l->tail = removed->prev;
    if(removed->prev != NULL)
        l->tail->next = NULL;
    void *value = removed->value;
    free(removed);
    l->size--;
    assert(l->size >= 0);
    return value;
}

// Return true if the list contains this pointer value
bool list_contains(list l, void *value, bool(*equal)(void *, void *)) {
    iterator it = it_begin(l);
    while(it_hasNext(it)) {
        if(equal(it_next(it), value)) {
            it_free(&it);
            return true;
        }
    }
    it_free(&it);
    return false;
}

// Get a specific item in the list given by equal
list list_get(list l, void *value, bool(*equal)(void *, void *)) {

    list results = list_New();
   
    iterator it = it_begin(l);
    while(it_hasNext(it)) {
        void *p = it_next(it);
        if(equal(p, value))
            list_add(results, p);
    }
    it_free(&it);
    
    if(list_size(results) == 0) {
        list_delete(results);
        return NULL;
    }
    
    return results;
}

// Get the first item in the list given by equal
void *list_getFirst(list l, void *value, bool(*equal)(void *, void *)) {
    iterator it = it_begin(l);
    while(it_hasNext(it)) {
        void *p = it_next(it);
        if(equal(p, value)) {
            it_free(&it);
            return p;
        }
    }
    it_free(&it);
    return NULL;
}

// Remove all ocurrences of the element with this address value
void* list_remove(list l, void *value, bool(*equal)(void *, void *)) {
    void *p = NULL;
    void *q = NULL;
    iterator it = it_begin(l);
    while(it_hasNext(it)) {
        p = it_next(it);
        if(equal(p, value)) {
            it_remove(it);
            q = p;
        }
    }
    it_free(&it);
    return q;
}

// Return the head item
void *list_head(list l) {
    return l->head->value;
}

// Return the tail item
void *list_tail(list l) {
    return l->tail->value;
}

// Delete a list, but not it's elements
void list_clear(list l) {
    item n = l->head;
    item prev;
    while(n != NULL) {
        prev = n;
        n = n->next;
        free(prev);
    }
    l->head = NULL;
    l->tail = NULL;
    l->size = 0;
}

// Delete an entire list
void list_delete(list l) {
    list_clear(l);
    free(l);
}

// Delete a list and the elements contained in the list
void list_deepDelete(list l, void(*deleteItem)(void *)) {
    iterator it = it_begin(l);
    while(it_hasNext(it))
        deleteItem(it_next(it));
    it_free(&it);
    list_delete(l);
}

// Return the number of elements in the list
int list_size(list l) {
    return l->size;
}

// Indicate if the list is empty
bool list_empty(list l) {
    return l->size == 0;
}

// Dump the contents of the list
void list_dump(list l, FILE *out, string(*labelStr)(void *)) {
    iterator it = it_begin(l);
    while(it_hasNext(it)) {
        void *p = it_next(it);
        fprintf(out, "%s\n", labelStr(p));
    }
    it_free(&it);
}

/*
 * Iterator methods
 */

// Return the iterator for the beginning of this list
iterator it_begin(list l) {
    //printf("beginning it %d %d\n", (int)l, (int)l->activeIt);
    iterator it = (iterator) chkalloc(sizeof(*it));
    it->l = l;
    it->next = l->head;
    it->curr = l->head;
    it->forward = true;
    l->activeIts++;
    return it;
}

// Return the iterator for the beginning of this list
iterator it_end(list l) {
    iterator it = (iterator) chkalloc(sizeof(*it));
    it->l = l;
    it->next = l->tail;
    it->curr = l->tail;
    it->forward = false;
    l->activeIts++;
    return it;
}

// Copy an iterator
iterator it_copy(iterator it) {
    iterator new = (iterator) chkalloc(sizeof(*new));
    new->l = it->l;
    new->next = it->next;
    new->curr = it->curr;
    new->forward = it->forward;
    new->l->activeIts++;
    return new;
}

/* Return if there is a next element. If there are no more elements, then delete
 * the iterator
 */
bool it_hasNext(iterator it) {
    assert(it != NULL);
    if(!it->forward) {
        it->next = it->curr->next;
        it->forward = true;
    }
    if(it->next == NULL) {
        return false;
    }
    return true;
}

// Return if there is a previous element
bool it_hasPrev(iterator it) {
    assert(it != NULL);
    if(it->forward) {
        it->next = it->curr->prev;
        it->forward = false;
    }
    if(it->next == NULL) {
        return false;
    }
    return true;
}

// Delete an iterator after use
void it_free(iterator *it) {
    //printf("delete it %d\n", (int)it->l);
    (*it)->l->activeIts--;
    free(*it);
    *it = NULL;
}

// Return the next element in the iterator direction of first-->last
void *it_next(iterator it) {
    assert(it != NULL);
    if(!it->forward) {
        it->next = it->curr->next;
        it->forward = true;
    }
    it->curr = it->next;
    it->next = it->next->next;
    return it->curr->value;
}

// Return the next element in the reverse direction
void *it_prev(iterator it) {
    assert(it != NULL);
    if(it->forward) {
        it->next = it->curr->prev;
        it->forward = false;
    }
    it->curr = it->next;
    it->next = it->next->prev;
    return it->curr->value;
}

void *it_curr(iterator it) {
    return it->curr->value;
}

// Return the next element in the iterator, but don't move forward
void *it_peekNext(iterator it) {
    assert(it != NULL);
    return it->curr->next->value;
}

// Return the next element in the iterator, but don't move forward
void *it_peekPrev(iterator it) {
    assert(it != NULL);
    return it->curr->prev->value;
}

// Replace the current entry in the list
void it_replace(iterator it, void *value) {
    assert(it != NULL);
    
    if(it->l->activeIts > 1) {
        err_fatal("cannot modify list while > 1 iterators active");
    }

    item before, after;
    
    if(it->forward) {
        before = it->curr->prev;
        after = it->curr->next;
    } else {
        before = it->curr->prev;
        after = it->curr->next;
    }
   
    // New
    item new = Node(value, before, after);

    // Brefore
    if(before != NULL) before->next = new;
    else it->l->head = new;
    
    // After
    if(after != NULL)  after->prev = new;
    else it->l->tail = new;
    
    // Curr
    it->curr = new;
}

// Insert an item before the position of the iterator
void it_insertBefore(iterator it, void *value) {
    assert(it != NULL);
    
    if(it->l->activeIts > 1) {
        err_fatal("cannot modify list while > 1 iterators active");
    }

    item before, after;
    if(it->forward) {
        before = it->curr->prev;
        after = it->curr;
    }
    else {
        before = it->curr;
        after = it->curr->next;
    }
    item new = Node(value, before, after);
    if(before != NULL) before->next = new;
    else it->l->head = new;
    if(after != NULL)  after->prev = new;
    else it->l->tail = new;
    it->l->size++;
}

// Insert an item before the position of the iterator
void it_insertAfter(iterator it, void *value) {
    assert(it != NULL);
    
    if(it->l->activeIts > 1) {
        err_fatal("cannot modify list while > 1 iterators active");
    }
    
    item before, after;
    if(!it->forward) {
        before = it->curr->prev;
        after = it->curr;
    }
    else {
        before = it->curr;
        after = it->curr->next;
    }
    item new = Node(value, before, after);
    if(before != NULL) before->next = new;
    else it->l->head = new;
    if(after != NULL)  after->prev = new;
    else it->l->tail = new;
    it->l->size++;
}

/* Remove the current item pointed to by the iterator. Afterwards, if available
 * move the iterator to the next element ready for it_next(). If NULL, then to
 * the previous element or NULL.
 */
void *it_remove(iterator it) {
    assert(it != NULL);

    if(it->l->activeIts > 1) {
        err_fatal(StringFmt("cannot modify list while %d iterators active",
                it->l->activeIts));
    }

    item n = (item) it->curr;
    assert(n!=NULL);
    item before = n->prev;
    item after = n->next;
    
    if(before != NULL)
        before->next = after;
    else it->l->head = after;
    
    if(after != NULL) 
        after->prev = before;
    else it->l->tail = before;
    
    // Place the iterator and look-ahead
    it->curr = NULL;
    it->next = it->forward ? after : before;

    void *value = n->value;
    free(n);
    it->l->size--;
    assert(it->l->size >= 0);
    return value;
}

