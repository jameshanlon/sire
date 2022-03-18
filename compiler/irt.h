#ifndef IRT_H
#define IRT_H

#include "list.h"
#include "label.h"
#include "set.h"
#include "temp.h"

typedef struct i_stmt_ *i_stmt;
typedef struct i_expr_ *i_expr;

typedef enum {
    i_plus, 
    i_minus, 
    i_mult, 
    i_div,
    i_rem,
    i_or, 
	i_and, 
    i_xor,
    i_lshift, 
    i_rshift,
    i_eq, 
    i_ne, 
    i_ls, 
    i_le,
    i_gr,
    i_ge
} t_binop;

// Memory base types
typedef enum {
    t_mem_abs,  // absolute
    t_mem_spo,  // sp[outgoing args]
    t_mem_spl,  // sp[locals]
    t_mem_spp,  // sp[preserved regs]
    t_mem_spi,  // sp[incoming args]
    t_mem_dp,   // dp[i]
    t_mem_cp,   // cp[i]
    t_mem_spa,  // &sp[i] 
    t_mem_dpa,  // &dp[i]
    t_mem_cpa   // &cp[i]
} t_memBase;

struct i_stmt_ {
    enum t_i_stmt {
        t_LABEL, 
        t_JUMP, 
        t_CJUMP, 
        t_MOVE,
        t_INPUT,
        t_OUTPUT,
        t_PAR,
        t_FORK,
        t_FORKSET,
        t_FORKSYNC,
        t_JOIN,
		t_PCALL,
        t_ON,
        t_CONNECT,
        t_RETURN,
        t_NOP,
        t_END
    } type;       
    
    union {
		struct {
            i_expr expr;
			i_expr then;
            i_expr other;
        } CJUMP;
		struct {
            i_expr dst;
            i_expr src;
        } MOVE;
		struct {
            i_expr dst;
            i_expr src;
        } IO;
		struct {
            i_expr proc;
            list args;
        } PCALL;
        struct {
            i_expr dest;
            i_expr pCall;
        } ON;
        struct {
            i_expr to, c1, c2;
        } CONNECT;
        struct {
            i_expr end;
            i_expr expr;
        } RETURN;
        struct {
            i_expr t1, t2, t3;
            list threads;
        } FORK;
        struct {
            i_expr sync;
            i_expr thread;
            i_expr space;
            label l;
        } FORKSET;
        struct {
            i_expr sync;
        } FORKSYNC;
        struct {
            bool master;
            i_expr t1;
            label exit;
        } JOIN;
        list PAR;
		label LABEL;
        i_expr JUMP;
	} u;

    // For liveness analysis
    set def;
    set use;
    set in;
    set out;

    // For linear scan reg allocation
    int pos;
    int reg;
};

struct i_expr_ {
    enum {
        t_BINOP, 
        t_MEM, 
        t_TEMP, 
        t_NAME,
		t_CONST, 
        t_FCALL,
        t_SYS
    } type;
    union {
		temp TEMP;
		label NAME;
	    unsigned int CONST;
        struct {
            t_memBase type;
            i_expr base;
            i_expr offset;
        } MEM;
        struct {
            t_binop op; 
            i_expr left;
            i_expr right;
        } BINOP;
		struct {
            i_expr func;
            list args;
        } FCALL;
        struct {
            i_expr name;
            i_expr value;
        } SYS;
	} u;
};

// Statements
i_stmt i_Label(label);
i_stmt i_Jump(i_expr);
i_stmt i_CJump(i_expr expr, i_expr then, i_expr other);
i_stmt i_Move(i_expr, i_expr);
i_stmt i_Input(i_expr, i_expr);
i_stmt i_Output(i_expr, i_expr);
i_stmt i_Fork(i_expr, i_expr, i_expr, list);
i_stmt i_ForkSet(i_expr, i_expr, i_expr, label);
i_stmt i_ForkSync(i_expr);
i_stmt i_Join(i_expr, bool, label);
i_stmt i_PCall(i_expr, list);
i_stmt i_Return(i_expr, i_expr);
i_stmt i_On(i_expr, i_expr);
i_stmt i_Connect(i_expr, i_expr, i_expr);
i_stmt i_Nop();
i_stmt i_End();

// Expressions
i_expr i_Binop(t_binop, i_expr, i_expr);
i_expr i_Mem(t_memBase, i_expr, i_expr);
i_expr i_Temp(temp);
i_expr i_Name(label);
i_expr i_Const(unsigned int);
i_expr i_FCall(i_expr, list);
i_expr i_Sys(i_expr, i_expr);

// Statement use sets 
set    i_getUseSet(i_stmt);

// Statement defs
set    i_getDefSet(i_stmt);

// Delete statements and expressions
void   i_deleteStmt(void *);
void   i_deleteExpr(void *);

#endif
