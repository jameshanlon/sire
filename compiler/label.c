#include <stdlib.h>
#include <string.h>
#include "table.h"
#include "label.h"

// A single label
struct label_ {
    string name;
    int pos; // a position index, used in codegen for forward and backwards jumps
};

// Data structure to record allocated temporaries and labels
struct labelMap_ {
    int count;
    table map;
};

// constructor
labelMap lblMap_New() {
    labelMap m = (labelMap) chkalloc(sizeof(*m));
    m->count = 0;
    m->map = tab_New();
    return m;
}

// Create a new numbered label
label lblMap_NewLabel(labelMap m) {
    assert(m != NULL && "map NULL");
    label l = (label) chkalloc(sizeof(*l));
    l->name = StringFmt(".L%d", m->count++);
    l->pos = -1;
    tab_insert(m->map, l->name, l);
    return l;
}

// Create a named label
label lblMap_NewNamedLabel(labelMap m, string name) {
   
    // It shouldn't already exist
    label l = tab_lookup(m->map, name);
    //if(l!=NULL) printf("%s\n", lbl_name(l));
    assert(l == NULL && "label already exists");
    
    l = (label) chkalloc(sizeof(*l));
    l->name = StringFmt("%s", name);
    l->pos = -1;
    
    tab_insert(m->map, l->name, l);
    //printf("inserted named label %s\n", name);
    return l;
}

// Return an existing named label
label lblMap_getNamed(labelMap m, string name) {
    assert(m != NULL && "label map NULL");
    //printf("looking for named label %s\n", name);
    if(tab_lookup(m->map, name) == NULL)
        assert(0 && "Label not found");
    return (label) tab_lookup(m->map, name);
}

// Compare the value of two labels
bool lbl_compare(label a, label b) {
    assert(a != NULL && b != NULL && "Label NULL");
    return streq(a->name, b->name);
}

// Get the string of a temp label
string lbl_name(label l) {
    assert(l != NULL && "label NULL");
    return l->name;
}

void lbl_setPos(label l, int pos) {
    assert(l != NULL && "Label NULL");
    l->pos = pos;
}

int lbl_pos(label l) {
    assert(l != NULL && "Label NULL");
    assert(l->pos != -1 && "Label position is -1");
    return l->pos;
}

void lbl_delete(label l) {
    free(l);
}

void lbl_rename(label l, string name) {
    l->name = name;
}
