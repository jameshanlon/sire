#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "astprinter.h"

#define SHOW_EMPTY_NODES 1
#define SHOW_PAREN       1 

// Local function prototypes
static void p_constDecls  (FILE *, a_constDecls);
static void p_portDecls   (FILE *, a_portDecls);
static void p_varDecls    (FILE *, a_varDecls);
static void p_procDecls   (FILE *, a_procDecls);
static void p_procDecl    (FILE *, a_procDecl);
static void p_formals     (FILE *, a_formals);
static void p_stmtSeq     (FILE *, int, a_stmtSeq);
static void p_stmtPar     (FILE *, int, a_stmtPar);
static void p_stmt        (FILE *, int, a_stmt);
static string exprList    (a_exprList);
static string expr        (a_expr);
static string expr_monadic(a_expr);
static string expr_diadic (a_expr);
static string elem        (a_elem);

static char opStr[][4] = {
    "", "-", "~", "+", "-", "*", "/", "%", "or", "and", "xor",
    "<<", ">>", "=", "~=", "<", "<=", ">", ">=" 
};

// Indent by depth d
static void indent(FILE *out, int d) {
    int i;
    for(i=0; i<d; i++)
        fprintf(out, "    ");
}

// Module
void p_module(FILE *out, a_module p) {
    switch(p->type) {
    case t_module_main:
        p_constDecls(out, p->consts); printLn(out);
        p_portDecls (out, p->ports);  printLn(out); 
        p_varDecls  (out, p->vars); 
        p_procDecls (out, p->procs); 
        break;
    default:
        assert(0 && "Invalid module type");
    }
}

//========================================================================
// Declarations
//========================================================================

// ConstDecls
static void p_constDecls(FILE *out, a_constDecls p) {
    if(p!=NULL) { 
        fprintf(out, "const ");
        for(; p!=NULL; p=p->next)
            fprintf(out, "%s := %d%s", p->decl->name->name, 
                    p->decl->value, p->next!=NULL?", ":"");
        fprintf(out, ";");
    }
}

// PortDecls
static void p_portDecls(FILE *out, a_portDecls p) {
    if(p!=NULL) {
        fprintf(out, "port ");
        for(; p!=NULL; p=p->next)
            fprintf(out, "%s := %d%s", p->decl->name->name, 
                    p->decl->num, p->next!=NULL?", ":"");
        fprintf(out, ";");
    }
}

// VarDecls
static void p_varDecls(FILE *out, a_varDecls p) {
    a_varDecls v;
    a_idList t;
        
    if(p!=NULL) fprintf(out, "var ");
    
    // For each set of type declartations
    for(v=p; v!=NULL; v=v->next) {
        a_varDecl decl = v->vars;

        // For each declaration of the type
        for(t=decl->ids; t!=NULL; t=t->next)
            fprintf(out, "%s%s", t->var->name->name, 
                    t->next!=NULL?", ":"");

        a_varType type = v->vars->type;
        fprintf(out, ": %s%s%s", 
                a_varTypeStr(type->type),
                type->expr!=NULL ? StringFmt("[%s]", expr(type->expr)) : "", 
                v->next!=NULL ? "; " : "");
    }
    
    if(p!=NULL) fprintf(out, ";");
}

// ProcDecls
static void p_procDecls(FILE *o, a_procDecls p) {
    for(; p!=NULL; p=p->tail) {
        fprintf(o, "\n");
        p_procDecl(o, p->head);
        fprintf(o, "\n");
    }
}

// ProcDecl
static void p_procDecl(FILE *o, a_procDecl p) {

    switch(p->type) {
    case t_procDecl_proc: fprintf(o, "proc "); break;
    case t_procDecl_func: fprintf(o, "func "); break;
    default: assert(0 && "Invalid procDecl type");
    }
    
    fprintf(o, "%s (", p->name->name);
    p_formals(o, p->formals);
    fprintf(o, ") is\n");
    if(p->varDecls!=NULL) {
        fprintf(o, "  "); p_varDecls(o, p->varDecls); fprintf(o, "\n");
    }
    p_stmt(o, 1, p->stmt);
}

//========================================================================
// Formals
//========================================================================

// Formals
static void p_formals(FILE *o, a_formals p) {
    a_paramDeclSeq f;
    
    // For each set of type declartations
    for(; p!=NULL; p=p->next) {

        // For each declaration of the type
        for(f=p->params; f!=NULL; f=f->next)
            fprintf(o, "%s%s", f->param->name->name, 
                    f->next!=NULL?", ":"");
        
        fprintf(o, ": %s%s", a_formalTypeStr(p->type), 
                p->next!=NULL?"; ":"");
    }
}

//========================================================================
// Statements
//========================================================================

// StmtSeq
static void p_stmtSeq(FILE *out, int d, a_stmtSeq p) {
    for(; p!=NULL; p=p->tail) {
        p_stmt(out, d, p->head);
        fprintf(out, "%s\n", p->tail!=NULL?"; ":"");
    }
}

// StmtPar
static void p_stmtPar(FILE *out, int d, a_stmtPar p) {
    for(; p!=NULL; p=p->tail) {
        p_stmt(out, d, p->head);
        fprintf(out, "%s\n", p->tail!=NULL?"| ":"");
    }
}

// Stmt
static void p_stmt(FILE *o, int d, a_stmt p) {
    switch(p->type) {
    case t_stmt_skip:
        indent(o, d); fprintf(o, "skip");
        break;
    case t_stmt_return:
        indent(o, d); fprintf(o, "return %s", expr(p->u.return_.expr));
        break;
    case t_stmt_if:
        indent(o, d); fprintf(o, "if %s\n", expr(p->u.if_.expr));
        indent(o, d); fprintf(o, "then\n");
        p_stmt(o, d+1, p->u.if_.stmt1);
        fprintf(o, "\n"); indent(o, d); fprintf(o, "else\n"); 
        p_stmt(o, d+1, p->u.if_.stmt2);
        break;
    case t_stmt_while:
        indent(o, d); fprintf(o, "while %s do\n", expr(p->u.while_.expr));
        p_stmt(o, d+1, p->u.while_.stmt);
        break;
    case t_stmt_for:
        indent(o, d); fprintf(o, "for %s := %s to %s do\n", elem(p->u.for_.var), 
                expr(p->u.for_.pre), expr(p->u.for_.post));
        p_stmt(o, d+1, p->u.for_.stmt);
        break;
    case t_stmt_pCall:
        indent(o, d); fprintf(o, "%s(%s)", p->u.pCall.name->name, 
                exprList(p->u.pCall.exprList));
        break;
    case t_stmt_ass:
        indent(o, d); 
        fprintf(o, "%s := %s", elem(p->u.ass.dst), expr(p->u.ass.src));
        break;
    case t_stmt_input:
        indent(o, d); 
        fprintf(o, "%s ? %s", elem(p->u.io.dst), expr(p->u.io.src));
        break;
    case t_stmt_output:
        indent(o, d); 
        fprintf(o, "%s ! %s", elem(p->u.io.dst), expr(p->u.io.src));
        break;
    case t_stmt_on:
        indent(o, d); 
        fprintf(o, "on %s : %s", 
                elem(p->u.on.dest), 
                elem(p->u.on.pCall));
        break;
    case t_stmt_alias:
        indent(o, d); 
        fprintf(o, "%s alias %s[%s..]", 
                elem(p->u.alias.dst),
                elem(p->u.alias.array), 
                expr(p->u.alias.index));
        break;
    case t_stmt_connect:
        indent(o, d); 
        fprintf(o, "connect %s to %s : %s",
                elem(p->u.connect.c1),
                elem(p->u.connect.to),
                elem(p->u.connect.c2));
        break;
    case t_stmt_par:
        indent(o, d-1); fprintf(o, "{\n"); 
        p_stmtPar(o, d, p->u.par);
        indent(o, d-1); fprintf(o, "}"); 
        break;
    case t_stmt_seq:
        indent(o, d-1); fprintf(o, "{\n"); 
        p_stmtSeq(o, d, p->u.seq);
        indent(o, d-1); fprintf(o, "}"); 
        break;
    default:
        assert(0 && "Invalid stmt type");
    }
}

//========================================================================
// Expressions
//========================================================================

// Exprlist
//TODO: stop these str mem leaks strDel(s); strDel(t); 
static string exprList(a_exprList p) {
    string s = "", t, u;
    for(; p!=NULL; p=p->tail) {
        t = StringFmt("%s", expr(p->head), p->tail!=NULL?", ":"");
        u = StringCat(s, t);
        s = u;
    }
    return s;
}

// Expr
static string expr(a_expr p) {
    switch(p->type) {
    case t_expr_none:   case t_expr_neg:     case t_expr_not:
        return expr_monadic(p);

    case t_expr_plus:   case t_expr_minus:   case t_expr_mult:
    case t_expr_div:    case t_expr_rem:     case t_expr_or:
    case t_expr_and:    case t_expr_xor:     case t_expr_lshift:
    case t_expr_rshift: case t_expr_eq:      case t_expr_ne:
    case t_expr_ls:     case t_expr_le:      case t_expr_gr:
    case t_expr_ge:
        return expr_diadic(p);

    default:
        assert(0 && "Invalid expr type");
    }
}

// Expr monadic
static string expr_monadic(a_expr p) {
    return StringFmt("%s%s", 
            opStr[p->type], 
            elem(p->u.monadic.elem));
}

// Expr diadic
static string expr_diadic(a_expr p) {
    return StringFmt("(%s %s %s)", 
            elem(p->u.diadic.elem),
            opStr[p->type], 
            expr(p->u.diadic.expr));
}

//========================================================================
// Elements
//========================================================================

// Element
static string elem(a_elem p) {
    switch(p->type) {
    case t_elem_name:    return StringFmt("%s", p->u.name->name);
    case t_elem_number:  return StringFmt("%d", p->u.number);
    case t_elem_boolean: return p->u.boolean ? "true" : "false";
    case t_elem_string:  return p->u.string.value;
    case t_elem_expr:    return expr(p->u.expr);
    case t_elem_sub:     return StringFmt("%s[%s]", p->u.sub.name->name, 
                                 expr(p->u.sub.expr));
    
    case t_elem_pCall:
        return StringFmt("%s(%s)", p->u.call.name->name,
                exprList(p->u.call.exprList));
    
    case t_elem_fCall:
        return StringFmt("%s(%s)", p->u.call.name->name,
                exprList(p->u.call.exprList));

    default:
        assert(0 && "Invalid element type");
    }
}

