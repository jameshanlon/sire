#ifndef IR_H
#define IR_H

#include <stdio.h>
#include "list.h"
#include "table.h"
#include "ast.h"
#include "frame.h"
#include "label.h"

typedef struct intRep_  *intRep;
typedef struct ir_port_ *ir_port;
typedef struct ir_proc_ *ir_proc;
typedef struct ir_data_ *ir_data;

struct intRep_ {
    int dpOff;
    int cpOff;
    list ports;
    list procs;
    list data;
    list consts;
};

struct ir_port_ {
    int num;
};

struct ir_proc_ {
    int pos;
    frame frm;
    list blocks;
    list children;
    union {
        a_stmt as;
        list ir;
    } stmts;
};

struct ir_data_ {
    enum { 
        t_ir_global, 
        t_ir_const, 
        t_ir_str 
    } type;
    string name;
    label l;
    int off;
    union {
        int size;
        unsigned int value; // constant value
        string strVal;      // string value
    } u;
};

// IR methods
intRep ir_New(void);
void   ir_NewPort(intRep, int);
void   ir_NewProc(intRep, frame, a_stmt);
void   ir_NewGlobal(intRep, string, int, label);
label  ir_NewConst(intRep, labelMap, int);
void   ir_NewString(intRep, string, label);
label  ir_dpLoc(intRep, string);
void   ir_delete(intRep);

list ir_childCalls(ir_proc);
void ir_addChildCall(intRep, string, string);

// Comparators
bool   isNamedProc(void *, void *);

// Printing methods
void   ir_display(intRep, FILE *);

#endif
