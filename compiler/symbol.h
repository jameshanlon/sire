#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdio.h>
#include "util.h"

typedef struct symbol_   *symbol;
typedef struct symTable_ *symTable;

// Symbol types
typedef enum {
    t_sym_undefined,
    t_sym_mark,
    t_sym_module,
    t_sym_proc,
    t_sym_func,
    t_sym_const,
    t_sym_chan,
    t_sym_chanend,
    t_sym_port,
    t_sym_int,
    t_sym_intArray,
    t_sym_chanArray,
    t_sym_coreArray,
    t_sym_intAlias,
//    t_sym_chanAlias,
    t_sym_intArrayRef,
//    t_sym_chanArrayRef
} t_symbol;

// Scope types
typedef enum {
    t_scope_undefined,
    t_scope_system,
    t_scope_module,
    t_scope_proc,
    t_scope_func
} t_scope;

// Symbol table methods 
symTable  symTab_New         (void);
symbol    symTab_scopedInsert(symTable, string, t_symbol, int);
symbol    symTab_lookup      (symTable, string);
void      symTab_beginScope  (symTable, t_scope);
void      symTab_endScope    (symTable);
symbol    symTab_scopedLookup(symTable, string key);
t_scope   symTab_currScope   (symTable);
void      symTab_delete      (symTable);
void      symTab_dump        (symTable, FILE *out);

// Symbol methods
string    sym_name       (symbol);
t_symbol  sym_type       (symbol);
t_scope   sym_scope      (symbol);
string    sym_typeStr    (symbol);
int       sym_constVal   (symbol);
void      sym_setConstVal(symbol, int);

string    sym_scopeTypeStr(t_scope);

#endif
