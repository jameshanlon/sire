#include <stdarg.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "label.h"
#include "irtprinter.h"

static char binopStr[][4] = {
    "+", 
    "-", 
    "*", 
    "/", 
    "rem", 
    "or", 
    "and", 
    "xor", 
    "<<", 
    ">>",
    "=", 
    "~=", 
    "<", 
    "<=", 
    ">", 
    ">="
};

static char memBaseStr[][5] = {
    "mem",
    "spo",
    "spl",
    "spp",
    "spi",
    "dp",
    "cp",
    "&sp",
    "&dp",
    "&cp"
};

static void print(FILE *out, int d, const string format, ...) {
    va_list ap;
    int i;
    for(i=0; i<d; i++) fprintf(out, "  ");
    va_start(ap, format);
    vfprintf(out, format, ap);
    va_end(ap);
}

void print_proc(FILE *out, ir_proc proc) {

    string formals = frm_formalStr(proc->frm);
    fprintf(out, "proc %s (%s) {\n", frm_name(proc->frm), formals);
    iterator it = it_begin(proc->stmts.ir);
    while(it_hasNext(it))
        p_stmt(out, 1, it_next(it));
    it_free(&it);
    fprintf(out, "}\n");
}

void print_fork(FILE *out, int d, i_stmt par) {
    print(out, d, "fork [%s, %s, %s] (", p_expr(par->u.FORK.t1), 
            p_expr(par->u.FORK.t2), p_expr(par->u.FORK.t3));
    iterator it = it_begin(par->u.FORK.threads);
    while(it_hasNext(it)) {
        label l = it_next(it);
        fprintf(out, "%s%s", lbl_name(l), it_hasNext(it) ? ", " : "");
    }
    it_free(&it);
    fprintf(out, ")\n");
}

void print_pCall(FILE *out, int d, i_stmt stmt) {
    print(out, d, "pcall %s (", p_expr(stmt->u.PCALL.proc)); 
    iterator it = it_begin(stmt->u.PCALL.args);
    while(it_hasNext(it)) {
        string e = p_expr(it_next(it));
        fprintf(out, "%s%s", e, it_hasNext(it) ? ", " : "");
    }
    fprintf(out, ")\n");
    it_free(&it);
}

// Print statement
void p_stmt(FILE *out, int d, i_stmt stmt) {
    
    if(stmt == NULL) {
        print(out, d, "NULL stmt\n");
        return;
    }

    switch(stmt->type) {

    case t_LABEL:
        print(out, d, "\n");
        print(out, d, "%s:\n", lbl_name(stmt->u.LABEL));
        break;

    case t_JUMP:  
        print(out, d+1, "jump %s\n", p_expr(stmt->u.JUMP));
        break;

    case t_CJUMP:
        print(out, d+1, "cjump %s %s %s\n",
            p_expr(stmt->u.CJUMP.expr), 
            p_expr(stmt->u.CJUMP.then),
            p_expr(stmt->u.CJUMP.other));
        break;

    case t_MOVE:
        print(out, d+1, "%s := %s\n",
            p_expr(stmt->u.MOVE.dst), 
            p_expr(stmt->u.MOVE.src));
        break;

    case t_INPUT:
        print(out, d+1, "%s ? %s\n",
            p_expr(stmt->u.IO.dst), 
            p_expr(stmt->u.IO.src));
        break;

    case t_OUTPUT:
        print(out, d+1, "%s ! %s\n",
            p_expr(stmt->u.IO.dst), 
            p_expr(stmt->u.IO.src));
        break;

    case t_FORK:
        print_fork(out, d+1, stmt);
        break;

    case t_FORKSET:
        print(out, d+1, "forkSet [%s, %s, %s]: %s\n", p_expr(stmt->u.FORKSET.sync),
            p_expr(stmt->u.FORKSET.thread), p_expr(stmt->u.FORKSET.space),
            lbl_name(stmt->u.FORKSET.l));
        break;

    case t_FORKSYNC:
        print(out, d+1, "forkSync [%s]\n", p_expr(stmt->u.FORKSYNC.sync));
        break;

    case t_JOIN:
        print(out, d+1, "join [%s]\n", p_expr(stmt->u.JOIN.t1));
        break;

    case t_PCALL:
        print_pCall(out, d+1, stmt); 
        break;

    case t_ON:
        print(out, d+1, "on %s : %s\n",
            p_expr(stmt->u.ON.dest),
            p_expr(stmt->u.ON.pCall));
        break;

    case t_CONNECT:
        print(out, d+1, "connect %s to %s : %s\n",
            p_expr(stmt->u.CONNECT.c1),
            p_expr(stmt->u.CONNECT.to),
            p_expr(stmt->u.CONNECT.c2));
        break;

    case t_RETURN: 
        print(out, d+1, "return %s %s\n", 
            p_expr(stmt->u.RETURN.end), 
            p_expr(stmt->u.RETURN.expr)); 
        break;

    case t_NOP:
        print(out, d+1, "nop\n");
        break;

    case t_END:
        print(out, d+1, "end\n");
        break;

    default: 
        assert(0 && "invalid stmt type");
    }
}

// Return a string represenation of an expression element
string p_expr(i_expr e) {
    
    switch (e->type) {
    
    case t_BINOP:
        return StringFmt("%s %s %s", 
            p_expr(e->u.BINOP.left),
            binopStr[e->u.BINOP.op], 
            p_expr(e->u.BINOP.right));

    case t_MEM:
        return StringFmt("%s[%s%s]", 
            memBaseStr[e->u.MEM.type],
            e->u.MEM.base != NULL ? 
                StringFmt("%s + ", p_expr(e->u.MEM.base)) : "",
            p_expr(e->u.MEM.offset));

    case t_TEMP:
        return tmp_name(e->u.TEMP);

    case t_NAME:
        return lbl_name(e->u.NAME);

    case t_CONST:
        return StringFmt("%d", e->u.CONST);

    case t_FCALL: {
        string s = StringFmt("fcall %s (", p_expr(e->u.FCALL.func));
        iterator it = it_begin(e->u.FCALL.args);
        while(it_hasNext(it)) {
            string e = p_expr(it_next(it));
            s = StringCat(s, StringFmt("%s%s", 
                    e, it_hasNext(it) ? ", " : ""));
        }
        s = StringCat(s, ")");
        it_free(&it);
        return s;
    }

    case t_SYS:
        return StringFmt("sys.%s[%s]", 
                p_expr(e->u.SYS.name),
                p_expr(e->u.SYS.value));

    default: assert(0 && "Invalid expr type");
    }
}
