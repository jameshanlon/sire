#include <stdlib.h>
#include "util.h"
#include "irtprinter.h"
#include "ir.h"

static bool cmpDataName(void *, void *);
static bool cmpConstVal(void *, void *);
//static bool cmpDataLblName(void *, void *);

// Constructor
intRep ir_New(void) {
    intRep p = (intRep) chkalloc(sizeof(*p));
    p->dpOff = 0;
    p->cpOff = 0;
    p->ports = list_New();
    p->procs = list_New();
    p->data = list_New();
    p->consts = list_New();
    return p;
}

// New port
void ir_NewPort(intRep ir, int num) {
    ir_port p = (ir_port) chkalloc(sizeof(*p));
    p->num = num;
    list_add(ir->ports, p);
}

// New procedure
void ir_NewProc(intRep ir, frame f, a_stmt stmts) {
    ir_proc p = (ir_proc) chkalloc(sizeof(*p));
    p->frm = f;
    p->stmts.as = stmts;
    p->blocks = NULL;
    p->children = list_New();
    p->pos = -1;
    list_add(ir->procs, p);
}

// New global variable, return an identifier for it
void ir_NewGlobal(intRep ir, string name, int size, label l) {
    assert(size != -1 && "global size -1");
    ir_data p = (ir_data) chkalloc(sizeof(*p));
    p->type = t_ir_global;
    p->name = name;
    p->u.size = size;
    p->l = l;
    p->off = ir->dpOff;
    list_add(ir->data, p);
    ir->dpOff += size;
    //printf("new global with label %s\n", lbl_name(l));
}

// New string value, return a symbolic identifier for it
void ir_NewString(intRep ir, string value, label l) {
    ir_data p = (ir_data) chkalloc(sizeof(*p));
    p->type = t_ir_str;
    p->name = lbl_name(l);
    p->l = l;
    p->u.strVal = value;
    p->off = ir->dpOff;
    list_add(ir->consts, p);
    ir->cpOff += strlen(value) / 4 + (strlen(value)%4 > 0 ? 1 : 0);
}

// New constant value to be spilled to memory and return an identifier for it
label ir_NewConst(intRep ir, labelMap lm, int value) {

    // See if it already exists
    ir_data c = list_getFirst(ir->consts, &value, &cmpConstVal);
    if(c != NULL)
        return c->l;

    // Otherwise create a new one
    label l = lblMap_NewNamedLabel(lm, 
            StringFmt(".const.%d", list_size(ir->consts)));
    ir_data p = (ir_data) chkalloc(sizeof(*p));
    p->type = t_ir_const;
    p->l = l;
    p->u.value = value;
    list_add(ir->consts, p);
    ir->cpOff += 1;
    return l;
}

// Return the offset to the DP of a global with a particular label name
label ir_dpLoc(intRep ir, string name) {
    ir_data g = list_getFirst(ir->data, name, &cmpDataName);
    return g->l;
}

// Return the offset to the DP of a global with a particular label name
/*int ir_dpOff(intRep ir, string name) {
    ir_data g = list_getFirst(ir->data, name, &cmpDataLblName);
    assert(g != NULL && "data element NULL");
    return g->off;
}*/

// Return the offset to the DP of a global with a particular label name
/*int ir_cpOff(intRep ir, string name) {
    ir_data g = list_getFirst(ir->consts, name, &cmpDataLblName);
    assert(g != NULL && "constant element NULL");
    return g->off;
}*/

// Return the position of a named procedure
int ir_procPos(intRep ir, string name) {
    ir_proc p = list_getFirst(ir->procs, name, &isNamedProc);
    assert(p != NULL && p->pos != -1 && "Invalid ir_proc");
    return p->pos;
}

// Return the list of child calls
list ir_childCalls(ir_proc p) {
    return p->children;
}

// Add a child procedure or function call
// void *: hack to get around a circular inclusion
void ir_addChildCall(intRep ir, string parent, string child) {
    
    ir_proc parentProc = list_getFirst(ir->procs, parent, &isNamedProc);
    ir_proc childProc = list_getFirst(ir->procs, child, &isNamedProc);
    
    if(!list_contains(parentProc->children, child, &isNamedProc)) {
        //printf("added child %s to %s\n", child, parent);
        list_add(parentProc->children, childProc);
    }
}

// Dump each frame 
void ir_dumpProcFrame(ir_proc proc, FILE *out) {
    frm_dump(out, proc->frm);
}

// Comparison for stirng value to proc name
bool isNamedProc(void *item, void *str) {
    return streq(frm_name(((ir_proc)item)->frm), str);
}

// Dump each proccess body irt
void ir_display(intRep ir, FILE *out) {
    iterator it = it_begin(ir->procs);
    while(it_hasNext(it)) {
        ir_proc proc = it_next(it);
        print_proc(out, proc); printf("\n");
        //frm_dump(out, proc->frm);
    }
    it_free(&it);
}

static bool cmpDataName(void *g, void *name) {
    return streq((((ir_data) g)->name), name);
}

/*static bool cmpDataLblName(void *g, void *name) {
    return streq(lbl_name(((ir_data) g)->l), name);
}*/

static bool cmpConstVal(void *c, void *value) {
    return ((ir_data) c)->u.value == *((unsigned int *) value);
}


