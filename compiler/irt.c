#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "irt.h"

// Statement uses
static void i_cJumpUses   (i_stmt, set);
static void i_moveUses    (i_stmt, set);
static void i_inputUses   (i_stmt, set);
static void i_outputUses  (i_stmt, set);
static void i_pcallUses   (i_stmt, set);
static void i_onUses      (i_stmt, set);
static void i_connectUses (i_stmt, set);
static void i_returnUses  (i_stmt, set);
static void i_forkSetUses (i_stmt, set);
static void i_forkSyncUses(i_stmt, set);
static void i_joinUses    (i_stmt, set);

// Statement defs
static temp i_moveDef   (i_stmt);
static temp i_inputDef  (i_stmt);
static void i_forkDef   (i_stmt, set);

// Expression use sets
static void binopUses   (i_expr, set use);
static void fnCallUses  (i_expr, set use);
static void memUses     (i_expr, set use);
static void sysUses     (i_expr, set use);

// ==================================================================
// IR node constructors
// ==================================================================

// Label
i_stmt i_Label(label label) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_LABEL;
    p->u.LABEL = label;
    return p;
}
 
// Jump
i_stmt i_Jump(i_expr label) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_JUMP;
    p->u.JUMP = label;
    return p;
}

// CJump
i_stmt i_CJump(i_expr expr, i_expr then, i_expr other) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_CJUMP;
    p->u.CJUMP.expr = expr;
    p->u.CJUMP.then = then;
    p->u.CJUMP.other = other;
    return p;
}
 
// Move
i_stmt i_Move(i_expr dst, i_expr src) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_MOVE;
    p->u.MOVE.dst = dst;
    p->u.MOVE.src = src;
    return p;
}
 
// Input
i_stmt i_Input(i_expr dst, i_expr src) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_INPUT;
    p->u.IO.dst = dst;
    p->u.IO.src = src;
    return p;
}
 
// Output
i_stmt i_Output(i_expr dst, i_expr src) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_OUTPUT;
    p->u.IO.dst = dst;
    p->u.IO.src = src;
    return p;
}

// Fork
i_stmt i_Fork(i_expr t1, i_expr t2, i_expr t3, list threads) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_FORK;
    p->u.FORK.threads = threads;
    p->u.FORK.t1 = t1;
    p->u.FORK.t2 = t2;
    p->u.FORK.t3 = t3;
    return p;
}

// ForkSet
i_stmt i_ForkSet(i_expr sync, i_expr thread, i_expr space, label l) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_FORKSET;
    p->u.FORKSET.sync = sync;
    p->u.FORKSET.thread = thread;
    p->u.FORKSET.space = space;
    p->u.FORKSET.l = l;
    return p;
}

// ForkSync
i_stmt i_ForkSync(i_expr sync) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_FORKSYNC;
    p->u.FORKSYNC.sync = sync;
    return p;
}

// Join
i_stmt i_Join(i_expr t1, bool master, label exit) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_JOIN;
    p->u.JOIN.t1 = t1;
    p->u.JOIN.master = master;
    p->u.JOIN.exit = exit;
    return p;
}

// PCall
i_stmt i_PCall(i_expr proc, list args) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_PCALL;
    p->u.PCALL.proc = proc;
    p->u.PCALL.args = args;
    return p;
}

// On
i_stmt i_On(i_expr dest, i_expr pCall) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_ON;
    p->u.ON.dest = dest;
    p->u.ON.pCall = pCall;
    return p;
}

// Connect
i_stmt i_Connect(i_expr to, i_expr c1, i_expr c2) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_CONNECT;
    p->u.CONNECT.to = to;
    p->u.CONNECT.c1 = c1;
    p->u.CONNECT.c2 = c2;
    return p;
}

// Return
i_stmt i_Return(i_expr end, i_expr expr) {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_RETURN;
    p->u.RETURN.end = end;
    p->u.RETURN.expr = expr;
    return p;
}

// Nop
i_stmt i_Nop() {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_NOP;
    return p;
}
 
// End
i_stmt i_End() {
    i_stmt p = (i_stmt) chkalloc(sizeof(*p));
    p->type = t_END;
    return p;
}
 
// Binop
i_expr i_Binop(t_binop op, i_expr left, i_expr right) {
    i_expr p = (i_expr) chkalloc(sizeof(*p));
    p->type = t_BINOP;
    p->u.BINOP.op = op;
    p->u.BINOP.left = left;
    p->u.BINOP.right = right;
    return p;
}
 
// Mem
i_expr i_Mem(t_memBase type, i_expr base, i_expr offset) {
    i_expr p = (i_expr) chkalloc(sizeof(*p));
    p->type = t_MEM;
    p->u.MEM.type = type;
    p->u.MEM.base = base;
    p->u.MEM.offset = offset;
    return p;
}
 
// Temp
i_expr i_Temp(temp t) {
    i_expr p = (i_expr) chkalloc(sizeof(*p));
    p->type = t_TEMP;
    p->u.TEMP = t;
    return p;
}
 
// Name
i_expr i_Name(label name) {
    i_expr p = (i_expr) chkalloc(sizeof(*p));
    p->type = t_NAME;
    p->u.NAME = name;
    return p;
}
 
// Const
i_expr i_Const(unsigned int consti) {
    i_expr p = (i_expr) chkalloc(sizeof(*p));
    p->type = t_CONST;
    p->u.CONST = consti;
    return p;
}
 
// Function call
i_expr i_FCall(i_expr func, list args) { 
    i_expr p = (i_expr) chkalloc(sizeof(*p));
    p->type = t_FCALL;
    p->u.FCALL.func = func;
    p->u.FCALL.args = args;
    return p;
}

// System component
i_expr i_Sys(i_expr name, i_expr value) {
    i_expr p = (i_expr) chkalloc(sizeof(*p));
    p->type = t_SYS;
    p->u.SYS.name = name;
    p->u.SYS.value = value;
    return p;
}

// Delete an expression
void i_deleteStmt(void *s) { free(s); }

// Delete an expression
void i_deleteExpr(void *e) { free(e); }

// ==================================================================
// Statement use sets
// ==================================================================

// Get the set of variables used in this statement
set i_getUseSet(i_stmt stmt) {
    set use = set_New(&tmp_cmpTemp);

    switch(stmt->type) {
    case t_CJUMP:    i_cJumpUses(stmt, use);   break;
    case t_MOVE:     i_moveUses(stmt, use);    break;
    case t_INPUT:    i_inputUses(stmt, use);   break;
    case t_OUTPUT:   i_outputUses(stmt, use);  break;
    case t_PCALL:    i_pcallUses(stmt, use);   break;
    case t_ON:       i_onUses(stmt, use);      break;
    case t_CONNECT:  i_connectUses(stmt, use); break;
    case t_RETURN:   i_returnUses(stmt, use);  break;
    case t_FORKSET:  i_forkSetUses(stmt, use); break;
    case t_FORKSYNC: i_forkSyncUses(stmt, use); break;
    case t_JOIN:     i_joinUses(stmt, use);    break;
    
    case t_FORK: case t_LABEL: case t_JUMP: case t_END:                             
        break;

    default: assert(0 && "Invalid stmtement type");
    }

    return use;
}

static void allUses(i_expr src, set use) {
    switch(src->type) {
    case t_TEMP:   set_add(use, src->u.TEMP); break; 
    case t_BINOP:  binopUses(src, use);       break;
    case t_FCALL:  fnCallUses(src, use);      break;
    case t_MEM:    memUses(src, use);         break;
    case t_SYS:    sysUses(src, use);         break;
    case t_NAME:                              break;
    case t_CONST:                             break;
    default: assert(0 && "invalid MOVE/I/O src");
    }
}

static void callUses(list args, set use) {
    iterator it = it_begin(args);
    while(it_hasNext(it)) {
        i_expr arg = it_next(it);
        assert(arg->type == t_TEMP && "CALL arg not TEMP");
        set_add(use, arg->u.TEMP);
    }
    it_free(&it);
}

static void i_cJumpUses(i_stmt s, set use) {
    i_expr cond = s->u.CJUMP.expr;
    assert(cond->type == t_TEMP && "CJUMP condition not TEMP");
    set_add(use, cond->u.TEMP);
}

static void i_moveUses(i_stmt s, set use) {
    i_expr src = s->u.MOVE.src;
    i_expr dst = s->u.MOVE.dst;
    allUses(src, use);
    if(dst->type == t_MEM)
        memUses(dst, use);
}

static void i_inputUses(i_stmt s, set use) {
    i_expr src = s->u.IO.src;
    i_expr dst = s->u.IO.dst;
    allUses(src, use);
    assert(dst->type == t_TEMP && "input dst not TEMP");
    set_add(use, dst->u.TEMP);
}

static void i_outputUses(i_stmt s, set use) {
    i_expr src = s->u.IO.src;
    i_expr dst = s->u.IO.dst;
    allUses(dst, use);
    assert(src->type == t_TEMP && "output dst not TEMP");
    set_add(use, src->u.TEMP);
}

static void i_pcallUses(i_stmt s, set use) {
    callUses(s->u.PCALL.args, use);
}

static void i_onUses(i_stmt s, set use) {
    allUses(s->u.ON.dest, use);
    i_expr fCall = s->u.ON.pCall;
    callUses(fCall->u.FCALL.args, use);
}

static void i_connectUses(i_stmt s, set use) {
    allUses(s->u.CONNECT.to, use);
    allUses(s->u.CONNECT.c1, use);
    allUses(s->u.CONNECT.c2, use);
}

static void i_returnUses(i_stmt s, set use) {
    allUses(s->u.RETURN.expr, use);
}

static void i_forkSetUses(i_stmt s, set use) {
    set_add(use, s->u.FORKSET.sync->u.TEMP);
    set_add(use, s->u.FORKSET.thread->u.TEMP);
    set_add(use, s->u.FORKSET.space->u.TEMP);
}

static void i_forkSyncUses(i_stmt s, set use) {
    set_add(use, s->u.FORKSYNC.sync->u.TEMP);
}

static void i_joinUses(i_stmt s, set use) {
    set_add(use, s->u.JOIN.t1->u.TEMP);
}

// ==================================================================
// Expression use sets
// ==================================================================

static void binopUses(i_expr e, set use) {
    i_expr op1 = e->u.BINOP.left;
    i_expr op2 = e->u.BINOP.right;

    // Check valid operands
    assert((op1->type == t_TEMP || op1->type == t_NAME || 
                op1->type == t_CONST) 
            && "invalid BINOP operand 1");
    assert((op2->type == t_TEMP || op2->type == t_CONST) 
            && "invalid BINOP operand 2");
    assert(!(op1->type == t_CONST && op2->type == t_CONST) 
            && "BINOP CONST CONST not valid");
    
    // Add any used temps
    if(op1->type == t_TEMP)
        set_add(use, op1->u.TEMP);

    if(op2->type == t_TEMP)
        set_add(use, op2->u.TEMP);
}

static void fnCallUses(i_expr e, set use) {
    callUses(e->u.FCALL.args, use);
}

static void memUses(i_expr e, set use) {

    i_expr base = e->u.MEM.base;
    i_expr off = e->u.MEM.offset;

    // Check the expression is valid
    assert((off->type == t_TEMP || off->type == t_CONST || off->type == t_NAME)
            && "MEM expr not TEMP, CONST, NAME");

    // Add any used temp
    if(off->type == t_TEMP)
        set_add(use, off->u.TEMP);

    // If the base expr is not null, add the temp
    if(base != NULL) {
        assert((   e->type == t_mem_spo 
                || e->type == t_mem_spl
                || e->type == t_mem_spp
                || e->type == t_mem_spi
                || e->type == t_mem_abs) 
                && "MEM base not NULL for cp or dp access");
        
        // Add it if it's a TEMP. (It could be a constant for a local var)
        if(base->type == t_TEMP)
            set_add(use, base->u.TEMP);
    }
}

static void sysUses(i_expr e, set use) {
    if(e->u.SYS.value->type == t_TEMP)
        set_add(use, e->u.SYS.value->u.TEMP);
}

// ==================================================================
// Statement defs
// ==================================================================

// Get the set of variables defined in this statement. Variables (TEMPS) or
// array variables (TEMPS in MEMs) can be assigned to. For output ops src and
// dst variables are switched around
set i_getDefSet(i_stmt stmt) {
    set def = set_New(&tmp_cmpTemp);

    temp t = NULL;
    switch(stmt->type) {
    
    case t_MOVE:  t = i_moveDef(stmt);  break;
    case t_INPUT: t = i_inputDef(stmt); break;
    case t_FORK:  i_forkDef(stmt, def); break;
    
    case t_FORKSET: case t_FORKSYNC: case t_JOIN:   
    case t_OUTPUT:  case t_CJUMP:    case t_PCALL:    
    case t_ON:      case t_CONNECT:  case t_RETURN:   
    case t_LABEL:   case t_JUMP:     case t_END:
        break;

    default: assert(0 && "invalid statement type");
    }
    
    if(t != NULL) {
        tmp_setAccessUnassigned(t);
        set_add(def, t);
    }

    return def;
}

static temp i_moveDef(i_stmt s) {
    i_expr dst = s->u.MOVE.dst;
    assert((dst->type == t_TEMP || dst->type == t_MEM) && 
            "move dst must be TEMP or MEM");
    if(s->u.MOVE.dst->type == t_TEMP)
        return dst->u.TEMP;

    return NULL;
}

static temp i_inputDef(i_stmt s) {
    i_expr dst = s->u.IO.dst;
    assert(dst->type == t_TEMP && "input dst not TEMP");
    return dst->u.TEMP;
}

static void i_forkDef(i_stmt s, set def) {
    set_add(def, s->u.FORK.t1->u.TEMP);
    set_add(def, s->u.FORK.t2->u.TEMP);
    set_add(def, s->u.FORK.t3->u.TEMP);
}

