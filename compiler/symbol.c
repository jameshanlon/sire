#include <stdlib.h>
#include <stdio.h>
#include "table.h"
#include "error.h"
#include "temp.h"
#include "symbol.h"
#include "ast.h"
#include "statistics.h"

#define NUM_SCOPES 3

// Symbol
struct symbol_ {
    string name;
    t_symbol type;
    t_scope scope;
    symbol prev;    // previous binding
    int constVal;
};

// Symbol table
struct symTable_ {
    table tab;
    symbol top;
    t_scope currScope;
};

static char symTypeStr[][10] = {
    "undefined",
    "mark",
    "module",
    "proc",
    "func",
    "chan",
    "chanend",
    "port",
    "integer",
    "int[]",
    "chan[]",
};

static char scopeTypeStr[NUM_SCOPES][9] = {
    "<module>",
    "<proc>",
    "<func>"
};

// Create a new symbol
static symbol Symbol(string name, t_symbol type, t_scope scope, symbol prev) {
    symbol s = chkalloc(sizeof(*s));
    s->name = name;
    s->type = type;
    s->scope = scope;
    s->prev = prev;
    return s;
}

// Constructor
symTable symTab_New(void) {
    symTable t = (symTable) chkalloc(sizeof(*t));
    t->tab = tab_New();
    t->top = NULL;
    return t;
}

// Insert a symbol into the table
static symbol insert(symTable t, string name, t_symbol type) {
    t->top = Symbol(name, type, t->currScope, t->top);
    tab_insert(t->tab, t->top->name, t->top);
    stat_numSymbols++;
    return t->top;
}

// Lookup a symbol in the current scope
symbol symTab_scopedLookup(symTable t, string key) {
    //printf("scopedlookup\n");
    symbol s;
    for(s=t->top; s!=NULL; s=s->prev) {
        if (s->type == t_sym_mark)
            return NULL;
        //printf("\t%s\n", s->name);
        if(streq(key, s->name)) return s;
    }
    return NULL;
}

// Lookup a symbol
symbol symTab_lookup(symTable t, string name) {
    return tab_lookup(t->tab, name);
}

// Insert a symbol only if it doesn't already exist
symbol symTab_scopedInsert(symTable t, string name, t_symbol type, int pos) {
    if(symTab_scopedLookup(t, name) == NULL) {
        //printf("Added %s in scope, type %d\n", name, type);
        return insert(t, name, type);
    } else {
        err_report(t_error, pos, "variable '%s' already defined in scope", name);
        return NULL;
    }
}

// Get the scope type for a given scope marker symbol
static t_scope getScopeType(string mark) {
    int i;
    for(i=0; i<NUM_SCOPES; i++) {
        if(streq(mark, scopeTypeStr[i]))
            return (t_scope) i;
    }
    return (t_scope) -1;
}

// Return the type of the current scope
static t_scope getCurrScope(symTable t) {
    symbol s;
    for(s=t->top; s!=NULL; s=s->prev) {
        if (s->type == t_sym_mark) {
            return getScopeType(s->name);
        }
    }
    return (t_scope) -1;
}

// Begin a variable scope
void symTab_beginScope(symTable t, t_scope s) {
    //printf("entered new scope %s\n", scopeTypeStr[s]);
    insert(t, scopeTypeStr[s], t_sym_mark);
    t->currScope = s;
}

// End a variable scope: pop symbols until scope beginning is found
// free symbol objects?
void symTab_endScope(symTable t) {
    symbol s;
    assert(t != NULL && "SymTable NULL");
    for(s=t->top; s!=NULL; s=s->prev) {
        tab_pop(t->tab, s->name);
        //printf("\tpopped %s\n", s->name);
        if(s->type == t_sym_mark)
            break;
    }
    t->top = s->prev;
    t->currScope = getCurrScope(t);
    //printf("left scope\n");
}

// Return the current scope
t_scope symTab_currScope(symTable t) {
    assert(t != NULL && "symTable NULL");
    return t->currScope;
}

// Dump all of the symbols from the table
void symTab_dump(symTable t, FILE *out) {
    symbol s = t->top;
    printf("%d symbols:\n", tab_size(t->tab));
    for(; s!=NULL; s=s->prev)
        fprintf(out, "\t%s %s\n", s->name, symTypeStr[s->type]);
}

// Return the name of a symbol
string sym_name(symbol s) {
    //assert(s != NULL && "symbol NULL");
    return s!=NULL ? s->name : NULL;
}

t_symbol sym_type(symbol s) {
    //assert(s != NULL && "symbol NULL");
    return s!=NULL ? s->type : t_sym_undefined;
}

t_scope sym_scope(symbol s) {
    //assert(s != NULL && "symbol NULL");
    return s!=NULL ? s->scope : t_scope_undefined;
}

string sym_typeStr(symbol s) {
    //assert(s != NULL && "symbol NULL");
    return s!=NULL ? symTypeStr[s->type] : symTypeStr[t_sym_undefined];
}

string sym_scopeTypeStr(t_scope s) {
    return scopeTypeStr[s];
}

int sym_constVal(symbol s) {
    //assert(s != NULL && "symbol NULL");
    return s->constVal;
}

void sym_setConstVal(symbol s, int val) {
    //assert(s != NULL && "symbol NULL");
    s->constVal = val;
}
