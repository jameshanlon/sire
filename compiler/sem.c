#include "error.h"
#include "../include/platform.h"
#include "../include/definitions.h"
#include "translate.h"
#include "sem.h"
#include "codegen.h"

#define MAX_CONST 65535
#define NUM_CONNECT_ARGS 3
#define NUM_MIGRATE_ARGS 1 // To save sp 

//TODO:
//  - functions should exit with a return
//  - no function side-effects: variable assignment or procedure calls

// TODO: disallow illegal use any SYS resources

static void addSysVars(structures);

// Semantic tree walking
static void module       (structures, a_module);
static void constDecls   (structures, a_constDecls);
static void portDecls    (structures, a_portDecls);
static void varDecls     (structures, frame, a_varDecls);
static void varDecl      (structures, frame, a_varId, t_varDecl, int);
static void procDecls    (structures, a_procDecls);
static void procDecl     (structures, a_procDecl);
static void formals      (structures, a_formals, list);
static frm_formal formal (structures, a_varId, t_formal);
static void stmt_seq     (structures, frame, a_stmtSeq);
static void stmt_par     (structures, frame, a_stmtPar);
static void stmt         (structures, frame, a_stmt);
static void expr         (structures, frame, a_expr);
static void monadic      (structures, frame, a_expr);
static void diadic       (structures, frame, a_expr);
static void elem         (structures, frame, a_elem);

// Variable declarations
static void varDecl_int      (structures, a_varId);
static void varDecl_chan     (structures, a_varId);
static void varDecl_chanArray(structures, a_varId, int);
static void varDecl_intArray (structures, a_varId, int);
static void varDecl_intAlias (structures, a_varId);
//static void varDecl_chanAlias(structures, a_varId);

// Statements 
static void module_main  (structures, a_module);
static void stmt_return  (structures, frame, a_stmt);
static void stmt_if      (structures, frame, a_stmt);
static void stmt_while   (structures, frame, a_stmt);
static void stmt_for     (structures, frame, a_stmt);
static void stmt_pCall   (structures, frame, a_stmt);
static void stmt_ass     (structures, frame, a_stmt);
static void stmt_io      (structures, frame, a_stmt);
static void stmt_on      (structures, frame, a_stmt);
static void stmt_alias   (structures, frame, a_stmt);
static void stmt_connect (structures, frame, a_stmt);

// Elements
static void elem_name    (structures, a_elem);
static void elem_call   (structures, frame, a_elem);
static void elem_sub     (structures, frame,  a_elem);
static void elem_string  (structures, a_elem);
               
// Constant value expression evaluation
static int evalExpr      (structures, a_expr);
static int evalElem      (structures, a_elem);
static int evalMonadic   (structures, a_expr);
static int evalDiadic    (structures, a_expr);

// Main method to perform semantic analysis on an AST
void analyse(structures s, a_module p) {
    symTab_beginScope(s->sym, t_scope_system);
    addSysVars(s);
    module(s, p);
    symTab_endScope(s->sym);
}

// Add system variables to the symbol table
static void addSysVars(structures s) {
   
    // core[]
    symbol sym = symTab_scopedInsert(s->sym, CORE_ARRAY, t_sym_coreArray, 0);
    sym_setConstVal(sym, NUM_CORES);
    lblMap_NewNamedLabel(s->lbl, CORE_ARRAY);
    
    // chan[]
    sym = symTab_scopedInsert(s->sym, CHAN_ARRAY, t_sym_chanArray, 0);
    sym_setConstVal(sym, NUM_PROG_CHANS);
    lblMap_NewNamedLabel(s->lbl, CHAN_ARRAY);
}

// Module
void module(structures s, a_module p) {
    if(p->type == t_module_main)
        module_main(s, p);
}

// Main module
static void module_main(structures s, a_module p) {
    symTab_beginScope(s->sym, t_scope_module);
    constDecls(s, p->consts);
    portDecls(s, p->ports);
    varDecls(s, NULL, p->vars);
    procDecls(s, p->procs);
    symTab_endScope(s->sym);

    // Check for 'main' procedure
    // TODO should check main is a procedure and not a function
    ir_proc mainProc = list_getFirst(s->ir->procs, "main", &isNamedProc);
    if(mainProc == NULL) {
        err_report(t_error, p->pos, 
                "main module does not contain a 'main' procedure");
        return;
    // If it exists, rename it to _main (there will be no other references to
    // main
    }
    else {
        frm_rename(mainProc->frm, LBL_MAIN);
    }
}

//========================================================================
// Const and port declarations
//========================================================================

// ConstDecls
static void constDecls(structures s, a_constDecls p) {
    
    // For each constant
    for(; p!=NULL; p=p->next) {
        a_constDecl d = p->decl;
        string name = d->name->name;
        symbol sym = symTab_scopedInsert(s->sym, name, t_sym_const, d->pos);
        d->value = evalExpr(s, d->expr);
        sym_setConstVal(sym, d->value);
    }
}

// PortDecls
static void portDecls(structures s, a_portDecls p) {
    
    // For each port
    for(; p!=NULL; p=p->next) {
        a_portDecl d = p->decl;
        string name = d->name->name;
        symbol sym = symTab_scopedInsert(s->sym, name, t_sym_port, d->pos);
        d->num = evalExpr(s, d->expr);
        sym_setConstVal(sym, d->num);
        ir_NewPort(s->ir, d->num);
    }
}

//========================================================================
// Variable declarations
//========================================================================

// Vardecls
static void varDecls(structures s, frame f, a_varDecls p) {
   
    a_idList d;

    for(; p!=NULL; p=p->next) {
        t_varDecl type = p->vars->type->type;
        switch(type) {

        case t_varDecl_int:         
        case t_varDecl_chan:
            for(d=p->vars->ids; d!=NULL; d=d->next)
                varDecl(s, f, d->var, type, 1);
            break;

        case t_varDecl_intArray:    
        case t_varDecl_chanArray:
            p->vars->type->len = evalExpr(s, p->vars->type->expr);
            for(d=p->vars->ids; d!=NULL; d=d->next)
                varDecl(s, f, d->var, type, p->vars->type->len);
            break;

        case t_varDecl_intAlias:
        //case t_varDecl_chanAlias:
            for(d=p->vars->ids; d!=NULL; d=d->next)
                varDecl(s, f, d->var, type, 1);
            break;

        default: assert(0 && "Invalid varDecl type");
        }
    }
}

// Vardecl
static void varDecl(structures s, frame f, a_varId p, t_varDecl type, int len) {
   
    switch(type) {
    case t_varDecl_int:       varDecl_int      (s, p);      break;
    case t_varDecl_chan:      varDecl_chan     (s, p);      break;
    case t_varDecl_intArray:  varDecl_intArray (s, p, len); break;
    case t_varDecl_chanArray: varDecl_chanArray(s, p, len); break;
    case t_varDecl_intAlias:  varDecl_intAlias (s, p);      break;
    //case t_varDecl_chanAlias: varDecl_chanAlias(s, p);      break;
    default: assert(0 && "Invalid varDecl type");
    }

    // Allocate the variable somewhere
    switch(symTab_currScope(s->sym)) {
    
    // Allocate the declaration as a global in the data section
    case t_scope_module:
        ir_NewGlobal(s->ir, p->name->name, len, lblMap_NewLabel(s->lbl));
        break;

    // Allocate space for local arrays
    case t_scope_proc:
    case t_scope_func:
        if(type == t_varDecl_intArray || type == t_varDecl_chanArray)
            frm_allocArray(f, p->name->name, len);
        break;

    default: assert(0 && "invalid scope");
    }
}

// Integer variable declaration
static void varDecl_int(structures s, a_varId p) {
    string name = p->name->name;
    symTab_scopedInsert(s->sym, name, t_sym_int, p->pos);
}

// Channel variable declaration
static void varDecl_chan(structures s, a_varId p) {
    string name = p->name->name;
    symTab_scopedInsert(s->sym, name, t_sym_chan, p->pos);
}

// Integer array variable declaration
static void varDecl_intArray(structures s, a_varId p, int len) {
    string name = p->name->name;
    symbol sym = symTab_scopedInsert(s->sym, name, t_sym_intArray, p->pos);
    sym_setConstVal(sym, len);
}

// Channel array variable declaration
static void varDecl_chanArray(structures s, a_varId p, int len) {
    string name = p->name->name;
    symbol sym = symTab_scopedInsert(s->sym, name, t_sym_chanArray, p->pos);
    sym_setConstVal(sym, len);
}

// Integer array variable declaration
static void varDecl_intAlias(structures s, a_varId p) {
    string name = p->name->name;
    symTab_scopedInsert(s->sym, name, t_sym_intAlias, p->pos);
}

// Channel array variable declaration
/*static void varDecl_chanAlias(structures s, a_varId p) {
    string name = p->name->name;
    symTab_scopedInsert(s->sym, name, t_sym_chanAlias, p->pos);
}*/

//========================================================================
// Procedure declarations
//========================================================================

// ProcDecls
static void procDecls(structures s, a_procDecls p) {
    if(p != NULL) {
        procDecl(s, p->head);
        procDecls(s, p->tail);
    }
}

// ProcDecl
static void procDecl(structures s, a_procDecl p) {
    string name = p->name->name;
    
    switch(p->type) {

    case t_procDecl_proc:
        symTab_beginScope(s->sym, t_scope_proc);
        sigTab_insert(s->sig, p);
        break;
    
    case t_procDecl_func:
        symTab_beginScope(s->sym, t_scope_func);
        sigTab_insert(s->sig, p);
        break;

    default: assert(0 && "Invalid procDecl type");
    }
    
    // Create a new Proc in the IR with frame and body objects
    list refs = list_New(); 
    formals(s, p->formals, refs);
    frame frm = frm_New(lblMap_NewNamedLabel(s->lbl, name), refs);
    varDecls(s, frm, p->varDecls);
    stmt(s, frm, p->stmt);
    ir_NewProc(s->ir, frm, p->stmt);
    symTab_endScope(s->sym);
}

//========================================================================
// Formals 
//========================================================================

// Formals
static void formals(structures s, a_formals p, list refs) {
    
    for(; p!=NULL; p=p->next) {
        t_formal type = p->type;
        a_paramDeclSeq a = p->params;

        for(; a!=NULL; a=a->next)
            list_add(refs, formal(s, a->param, type));
    }
}

// Formal, return whether variable escapes (passed by reference)
// All constant value parameters
static frm_formal formal(structures s, a_varId p, t_formal type) {
    string name = p->name->name;

    switch(type) {
    
    case t_formal_int:
        symTab_scopedInsert(s->sym, name, t_sym_int, p->pos);
        return frm_Formal(name, t_formal_int);
    
    case t_formal_port:
        symTab_scopedInsert(s->sym, name, t_sym_port, p->pos);
        return frm_Formal(name, t_formal_port);
    
    case t_formal_chanend:
        symTab_scopedInsert(s->sym, name, t_sym_chanend, p->pos);
        return frm_Formal(name, t_formal_chanend);

    case t_formal_intArray:
        symTab_scopedInsert(s->sym, name, t_sym_intArrayRef, p->pos);
        return frm_Formal(name, t_formal_intArray);

    default:
        assert(0 && "Invalid formal type");
    }
}

// =======================================================================
// Statements
// =======================================================================

// StmtSeq
static void stmt_seq(structures s, frame f, a_stmtSeq p) {
    if(p != NULL) {
        stmt(s, f, p->head);
        stmt_seq(s, f, p->tail);
    }
}

// StmtPar
static void stmt_par(structures s, frame f, a_stmtPar p) {
    if(p != NULL) {
        stmt(s, f, p->head);
        stmt_par(s, f, p->tail);
    }
}

// Stmt
static void stmt(structures s, frame f, a_stmt p) {
    switch(p->type) {
    case t_stmt_skip:                               break;
    case t_stmt_return:   stmt_return(s, f, p);     break;
    case t_stmt_if:       stmt_if(s, f, p);         break;
    case t_stmt_while:    stmt_while(s, f, p);      break;
    case t_stmt_for:      stmt_for(s, f, p);        break;
    case t_stmt_pCall:    stmt_pCall(s, f, p);      break;
    case t_stmt_ass:      stmt_ass(s, f, p);        break;
    case t_stmt_input:    stmt_io(s, f, p);         break;
    case t_stmt_output:   stmt_io(s, f, p);         break;
    case t_stmt_on:       stmt_on(s, f, p);         break;
    case t_stmt_alias:    stmt_alias(s, f, p);      break;
    case t_stmt_connect:  stmt_connect(s, f, p);    break;
    case t_stmt_seq:      stmt_seq(s, f, p->u.seq); break;
    case t_stmt_par:      stmt_par(s, f, p->u.par); break;
    default: assert(0 && "Invalid stmt type");
    }
}

// Return statement
static void stmt_return(structures s, frame f, a_stmt p) {
    expr(s, f, p->u.return_.expr);
}

// If statement
static void stmt_if(structures s, frame f, a_stmt p) {
    expr(s, f, p->u.if_.expr);
    stmt(s, f, p->u.if_.stmt1);
    stmt(s, f, p->u.if_.stmt2);
}

// While statement
static void stmt_while(structures s, frame f, a_stmt p) {
    expr(s, f, p->u.while_.expr);
    stmt(s, f, p->u.while_.stmt);
}

// For statement
static void stmt_for(structures s, frame f, a_stmt p) {
    elem(s, f, p->u.for_.var);
    expr(s, f, p->u.for_.pre);
    expr(s, f, p->u.for_.post);
    stmt(s, f, p->u.for_.stmt);
}

// Check procedure call statements have a matching signature
static void stmt_pCall(structures s, frame f, a_stmt p) {
    
    string name = p->u.pCall.name->name;
    
    // Check if symbol exists
    signature sig = sigTab_lookup(s->sig, name);
    if(sig == NULL) {
        err_report(t_error, p->pos, "undefined procedure: %s", name);
        return;
    }

    // Run through each arg
    a_exprList args = p->u.pCall.exprList;
    for(; args!=NULL; args=args->tail)
        expr(s, f, args->head);

    // Check if there is a matching signature
    if(!sigTab_checkArgs(s->sig, s->sym, name, p->u.pCall.exprList)) {
        err_report(t_error, p->pos, "undefined procedure reference: %s", name);
        return;
    }

    frm_pCallArgs(f, sigTab_numArgs(s->sig, name));
}

// Check that elem is a variable (var or sub) that can be assigned to
static void stmt_ass(structures s, frame f, a_stmt p) {
   
    elem(s, f, p->u.ass.dst);
    expr(s, f, p->u.ass.src);
    
    a_elem e = p->u.ass.dst;
    symbol sym = symTab_lookup(s->sym, e->u.name->name);

    //printf("checking assignment to %s\n", e->u.name.name->name);
    switch(e->type) {
    
    case t_elem_name:
        if(sym_type(sym) != t_sym_int) {
            err_report(t_error, p->pos, "cannot assign to variable of type %s", 
                    sym_typeStr(sym));
            return;
        }
        break;
    
    case t_elem_sub:
        if(sym_type(sym) != t_sym_intArray && sym_type(sym) !=
                t_sym_intArrayRef) {
            err_report(t_error, p->pos, "variable %s not of type array", 
                    e->u.sub.name);
            return;
        }
        break;
    
    default:
        err_report(t_error, p->pos, "cannot assign to %s\n", a_elemTypeStr(e));
        return;
    }
}

// Check left elem is a channel, timer or port, or resp
static void stmt_io(structures s, frame f, a_stmt p) {
    
    a_elem e = p->u.io.dst;
    elem(s, f, p->u.io.dst);
    expr(s, f, p->u.io.src);
                
    switch(e->type) {
    case t_elem_name: {
        symbol sym = symTab_lookup(s->sym, e->u.name->name);
        if(sym_type(sym) != t_sym_chanend && sym_type(sym) != t_sym_port) { 
            err_report(t_error, p->pos, 
                    "i/o operator cannot act on variable of type %s", 
                    sym_typeStr(sym));
            return;
        }
        break;
    }
    case t_elem_sub: {
        symbol sym = symTab_lookup(s->sym, e->u.name->name);
        if(sym_type(sym) != t_sym_chanArray) { 
            err_report(t_error, p->pos, 
                    "i/o operator cannot act on array variable of type %s", 
                    sym_typeStr(sym));
            return;
        }
        break;
    }
    default:
        err_report(t_error, p->pos, "i/o operator cannot act on %s",
                a_elemTypeStr(e));
        return;
    }
}

// On statement
static void stmt_on(structures s, frame f, a_stmt p) {

    // Check the destination expression is valid
    elem(s, f, p->u.on.dest);
  
    // TODO: check the dest is an array element
    a_name destArray = p->u.on.dest->u.name;
    symbol sym = symTab_lookup(s->sym, destArray->name);
    if(sym_type(sym) != t_sym_coreArray) {
        err_report(t_error, p->pos, "variable not of type core[]");
        return;
    }

    // Check the statement is a procedure call
    if(p->u.on.pCall->type != t_elem_pCall) {
        err_report(t_error, p->pos, 
                "'on' statement must be a procedure call");
        return;
    }

    // Check the call
    elem_call(s, f, p->u.on.pCall);

    // Make some preservation space for call to migrate
    frm_pCallArgs(f, NUM_MIGRATE_ARGS);
}

// Alias statement
// <dest> aliases <array>[<index>..]
// <dest> : arrayRef or arrayAlias
// <array> : array or arrayRef
static void stmt_alias(structures s, frame f, a_stmt p) {
    
    a_name dest = p->u.alias.dst->u.name;
    a_name array = p->u.alias.array->u.name;
  
    // Check they are defined
    elem(s, f, p->u.alias.dst);
    elem(s, f, p->u.alias.array);

    // Check the variable to assign to
    symbol sym = symTab_lookup(s->sym, dest->name);
    if(sym_type(sym) != t_sym_intAlias && sym_type(sym) !=
            t_sym_intArrayRef) {
        err_report(t_error, p->pos, 
                "variable %s not of type array alias or reference", 
                dest->name);
        return;
    }
    
    // Check the array to alias
    sym = symTab_lookup(s->sym, array->name);
    if(sym_type(sym) != t_sym_intArray && sym_type(sym) !=
            t_sym_intArrayRef) {
        err_report(t_error, p->pos, 
                "variable %s not of type array or reference", 
                array->name);
        return;
    }
    
    // Check the array expression
    expr(s, f, p->u.alias.index);
}

// Connect statement
static void stmt_connect(structures s, frame f, a_stmt p) {

    a_name to = p->u.connect.to->u.name;
    a_name c1 = p->u.connect.c1->u.name;
    a_name c2 = p->u.connect.c2->u.name;

    elem(s, f, p->u.connect.to);
    elem(s, f, p->u.connect.c1);
    elem(s, f, p->u.connect.c2);
   
    // Check to, c1, c2 are all arrays of the correct type
    symbol sym = symTab_lookup(s->sym, to->name);
    if(sym_type(sym) != t_sym_coreArray) {
        err_report(t_error, p->pos, "variable not of type core[]");
        return;
    }

    sym = symTab_lookup(s->sym, c1->name);
    if(sym_type(sym) != t_sym_chanArray) {
        err_report(t_error, p->pos, "variable not of type chan[]");
        return;
    }

    sym = symTab_lookup(s->sym, c2->name);
    if(sym_type(sym) != t_sym_chanArray) {
        err_report(t_error, p->pos, "variable not of type chan[]");
        return;
    }

    // Allocate space for the arguments to connect
    frm_pCallArgs(f, NUM_CONNECT_ARGS);
}

// =======================================================================
// Expressions
// =======================================================================

// Expr
static void expr(structures s, frame f, a_expr p) {
    if(p == NULL) return;

    switch(p->type) {
    case t_expr_none:   case t_expr_neg:   case t_expr_not:
        monadic(s, f, p);
        break;
    
    case t_expr_plus:   case t_expr_minus: case t_expr_mult:
    case t_expr_div:    case t_expr_rem:   case t_expr_or:
    case t_expr_and:    case t_expr_xor:   case t_expr_lshift:
    case t_expr_rshift: case t_expr_eq:    case t_expr_ne:
    case t_expr_ls:     case t_expr_le:    case t_expr_gr:
    case t_expr_ge: 
        diadic(s, f, p);
        break;
    
    default: assert(0 && "Invalid varDecl type");
    }
}

// Expr monadic
static void monadic(structures s, frame f, a_expr p) {
    elem(s, f, p->u.monadic.elem);
}

// Expr diadic
static void diadic(structures s, frame f, a_expr p) {
    elem(s, f, p->u.diadic.elem);
    expr(s, f, p->u.diadic.expr);
}

// =======================================================================
// Elements
// =======================================================================

// Element
static void elem(structures s, frame f, a_elem p) {
    if(p == NULL) return;
    switch(p->type) {
    case t_elem_name:    elem_name(s, p);       break;
    case t_elem_fCall:   elem_call(s, f, p);    break;
    case t_elem_sub:     elem_sub(s, f, p);     break;
    case t_elem_number:                         break;
    case t_elem_boolean:                        break;
    case t_elem_string:  elem_string(s, p);     break;
    case t_elem_expr:    expr(s, f, p->u.expr); break;
    default: assert(0 && "Invalid varDecl type");
    }
}

// Check a variable has been defined
static void elem_name(structures s, a_elem p) {
    string name = p->u.name->name;

    // Check if the symbol exists
    symbol sym = symTab_lookup(s->sym, name);
    if(sym == NULL) {
        err_report(t_error, p->pos, "undefined variable: %s", name);
        return;
    }

    p->sym = sym;
}

// Check function in scope and arguments match the signature
static void elem_call(structures s, frame f, a_elem p) {
    string name = p->u.call.name->name;
    
    // Check if symbol exists
    signature sig = sigTab_lookup(s->sig, name);
    if(sig == NULL) {
        err_report(t_error, p->pos, "undefined function: %s", name);
        return;
    }

    // Check if there is a matching signature
    if(!sigTab_checkArgs(s->sig, s->sym, name, p->u.call.exprList)) {
        err_report(t_error, p->pos, "invalid parameters for function %s", name);
        return;
    }

    // Run through each arg
    a_exprList args = p->u.call.exprList;
    for(; args!=NULL; args=args->tail)
        expr(s, f, args->head);

    frm_pCallArgs(f, sigTab_numArgs(s->sig, name));
}

// Array subscript
static void elem_sub(structures s, frame f, a_elem p) {
    
    string name = p->u.sub.name->name;
    expr(s, f, p->u.sub.expr);

    // Check if symbol exists
    p->sym = symTab_lookup(s->sym, name);
    if(p->sym == NULL) {
        err_report(t_error, p->pos, "undefined array variable: %s", name);
        return;
    }

    // Check symbol of correct type
    if(sym_type(p->sym) != t_sym_intArray 
        && sym_type(p->sym) != t_sym_intArrayRef
        && sym_type(p->sym) != t_sym_chanArray
        && sym_type(p->sym) != t_sym_coreArray) {
        err_report(t_error, p->pos, "variable not of type array");
        return;
    }
}

// Place string in pool, and assign it an identifier
static void elem_string(structures s, a_elem p) {
    label l = lblMap_NewLabel(s->lbl);
    ir_NewString(s->ir, p->u.string.value, l);
    p->u.string.l = l;
    //printf("created string label %s\n", lbl_name(l));
}

// ==================================================================
// Expression evaluation
// ==================================================================

// Evaluate an expression for a const val, array size or port width
static int evalExpr(structures s, a_expr p) {
    switch(p->type) {

    case t_expr_none:   case t_expr_neg:   case t_expr_not:
        return evalMonadic(s, p);
    
    case t_expr_plus:   case t_expr_minus: case t_expr_mult:
    case t_expr_div:    case t_expr_rem:   case t_expr_or:
    case t_expr_and:    case t_expr_xor:   case t_expr_lshift:
    case t_expr_rshift: case t_expr_eq:    case t_expr_ne:
    case t_expr_ls:     case t_expr_le:    case t_expr_gr:
    case t_expr_ge:
        return evalDiadic(s, p);
    
    default: assert(0 && "Invalid varDecl type");
    }
}

// Evaluate monadic expr
int evalMonadic(structures s, a_expr p) {
    int a = evalElem(s, p->u.monadic.elem);
    switch(p->type) { 
    case t_expr_none: return a;
    case t_expr_neg:  return -a;
    case t_expr_not:  return !a;
    default: assert(0 && "Invalid varDecl type");
    }
}

// Evaluate diadic expr
int evalDiadic(structures s, a_expr p) {
    int a = evalElem(s, p->u.diadic.elem);
    int b = evalExpr(s, p->u.diadic.expr);
    return trl_evalOp(p->type, a, b);
}

// Evaluate element value
static int evalElem(structures s, a_elem p) {
    switch(p->type) {

    // Unpermitted elements
    case t_elem_fCall:
        err_report(t_error, p->pos, 
                "use of function call '%s' in constant expression",
                p->u.call.name->name);
        return 0;

    case t_elem_string:
        err_report(t_error, p->pos, 
                "use of string '%s' in constant expression",
                p->u.string.value);
        return 0;
    
    case t_elem_sub:
        err_report(t_error, p->pos, 
                "use of array '%s' in constant expression",
                p->u.sub.name->name);
        return 0;

    // Permitted elements, name must be a constant value
    case t_elem_name: {
        string name = p->u.name->name;
        symbol sym = symTab_lookup(s->sym, name);
        if(sym != NULL) {
            if(sym_type(sym) != t_sym_const) {
                err_report(t_error, p->pos, 
                        "variable %s must be a constant value", name);
            }
            else return sym_constVal(sym);
        }
        err_report(t_error, p->pos, "variable %s is not defined", name);
        return 0;
    }
    
    case t_elem_number:
        return p->u.number;
    
    case t_elem_boolean: 
        return (int) p->u.boolean ? TRUE : FALSE;
    
    case t_elem_expr:    
        return evalExpr(s, p->u.expr);

    default: assert(0 && "Invalid varDecl type");
    }
}

