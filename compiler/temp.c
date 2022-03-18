#include <stdlib.h>
#include <string.h>
#include "table.h"
#include "frame.h"
#include "temp.h"

struct temp_ {
    string name;
    t_temp type;

    // For register allocation
    t_tmpAccess accessType;
    union {
        int reg;
        int off;
    } access;
    
    bool spill;
    string spilled;
};

// Data structure to record allocated temporaries
struct tempMap_ {
    int idCount;
    table map;
};

// constructor
tempMap tmp_New() {
    tempMap m = (tempMap) chkalloc(sizeof(*m));
    m->idCount = 0;
    m->map = tab_New();
    return m;
}

// Create a new temp for a given variable name if it doesn't already exist
temp tmp_Temp(tempMap m, string name, t_temp type) {
    temp t = tmp_lookup(m, name);
    if(t == NULL) {
        temp p = (temp) chkalloc(sizeof(*p));
        p->name = name;
        p->type = type;
        p->accessType = t_tmpAccess_undefined;
        p->access.reg = -1;
        p->access.off = -1;
        p->spill = false;
        p->spilled = NULL;
        tab_insert(m->map, name, p);
        //printf("inserted %s into tempMap\n", name);
        return p;
    }
    else return t;
}

// Lookup a temp value with a name key
temp tmp_lookup(tempMap m, string name) {
    //printf("lookinig up %s in tempMap\n");
    return tab_lookup(m->map, name);
}

// Return a new variable name, used for tempaorary allocations (not as in a
// temporary register allocation)
string tmp_NewName(tempMap m) {
    return StringFmt(".T%d", m->idCount++); 
}

// Return the string representation of a temp
string tmp_labelStr(void *t) {
    return StringFmt("%s", ((temp)t)->name);
}

void tmp_setRegAccess(temp t, int reg) { 
    t->accessType = t_tmpAccess_reg;
    t->access.reg = reg;
}

void tmp_setFrameAccess(temp t, int off) { 
    t->accessType = t_tmpAccess_frame;
    t->access.off = off;
}

void tmp_setCallerAccess(temp t, int off) { 
    t->accessType = t_tmpAccess_caller;
    t->access.off = off;
}

void tmp_setDataAccess(temp t) { 
    t->accessType = t_tmpAccess_data;
}

void tmp_setAccessNone(temp t) {
    t->accessType = t_tmpAccess_none;
}

void tmp_setAccessUnassigned(temp t) {
    t->accessType = t_tmpAccess_none;
}

void tmp_setSpilled(temp t, string name) {
    t->spill = true;
    t->spilled = name;
}

bool tmp_spill(temp t) {
    return t->spill;
}

string tmp_spilled(temp t) {
    return t->spilled;
}

string tmp_str(void *t) { 
    return tmp_name(t);
}

// Temp-name comparator
bool tmp_cmpName(void *t, void *name) {
    return streq(tmp_name((temp) t), name);
}

// Temp-temp comparator
bool tmp_cmpTemp(void *t1, void *t2) {
    return streq(tmp_name(t1), tmp_name(t2));
}


t_tmpAccess tmp_getAccess(temp t) { return t->accessType; }
int         tmp_reg(temp t)       { return t->access.reg; }
int         tmp_off(temp t)       { return t->access.off; }
string      tmp_name(temp t)      { return t->name; }
t_temp      tmp_type(temp t)      { return t->type; }
