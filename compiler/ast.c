#include "ast.h"

static char varTypeStr[][7] = {
    "chan",
    "int",
    "chan",
    "int",
};

static char formalTypeStr[][9] = {
    "int",
    "port",
    "chanend",
    "int[]"
};

static char elemTypeStr[][7] = {
    "name",
    "fnCall",
    "number",
    "bool",
    "string",
    "expr"
};

//========================================================================
// Program/module
//========================================================================

// Module_main
a_module a_module_Main(int pos, a_constDecls consts, a_portDecls ports, 
        a_varDecls vars, a_procDecls procs) {
    a_module p = chkalloc(sizeof(*p));
    p->type = t_module_main;
    p->pos = pos;
    p->consts = consts;
    p->ports = ports;
    p->vars = vars;
    p->procs = procs;
    return p;
}

//========================================================================
// Constant declarations
//========================================================================

// ConstDecls
a_constDecls a_ConstDecls(a_constDecl decl, a_constDecls next) {
    a_constDecls p = chkalloc(sizeof(*p));
    p->decl = decl;
    p->next = next;
    return p;
}

// ConstDecl
a_constDecl a_ConstDecl(int pos, a_name name, a_expr expr) {
    a_constDecl p = chkalloc(sizeof(*p));
    p->pos = pos;
    p->name = name;
    p->expr = expr;
    return p;
}

//========================================================================
// Port declarations
//========================================================================

// PortDecls
a_portDecls a_PortDecls(a_portDecl decl, a_portDecls next) {
    a_portDecls p = chkalloc(sizeof(*p));
    p->decl = decl;
    p->next = next;
    return p;
}

// PortDecl
a_portDecl a_PortDecl(int pos, a_name name, a_expr expr) {
    a_portDecl p = chkalloc(sizeof(*p));
    p->pos = pos;
    p->name = name;
    p->expr = expr;
    return p;
}

//========================================================================
// Variable declarations
//========================================================================

// VarDecls
a_varDecls a_VarDecls(a_varDecl varDecl, a_varDecls next) {
    a_varDecls p = chkalloc(sizeof(*p));
    p->vars = varDecl;
    p->next = next;
    return p;
}

// VarDecl
a_varDecl a_VarDecl(int pos, a_idList ids, a_varType type) {
    a_varDecl p = chkalloc(sizeof(*p));
    p->pos = pos;
    p->ids = ids;
    p->type = type;
    return p;
}

// VarType
a_varType a_VarType(int pos, t_varDecl type, a_expr expr) {
    a_varType p = chkalloc(sizeof(*p));
    p->pos = pos;
    p->type = type;
    p->expr = expr;
    return p;
}

// IdList
a_idList a_IdList(a_varId var, a_idList next) {
    a_idList p = chkalloc(sizeof(*p));
    p->var = var;
    p->next = next;
    return p;
}

// VarId
a_varId a_VarId(int pos, a_name name) {
    a_varId p = chkalloc(sizeof(*p));
    p->pos = pos;
    p->name = name;
    return p;
}

//========================================================================
// Procedure declarations
//========================================================================

// procDecls
a_procDecls a_ProcDecls(a_procDecl head, a_procDecls tail) {
    a_procDecls p = chkalloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

// procDecl_proc
a_procDecl a_procDecl_Proc(int pos, a_name name, a_formals formals,
        a_varDecls varDecls, a_stmt stmt) {
    a_procDecl p = chkalloc(sizeof(*p));
    p->type = t_procDecl_proc;
    p->pos = pos;
    p->name = name;
    p->formals = formals;
    p->varDecls = varDecls;
    p->stmt = stmt;
    return p;
}

// procDecl_func
a_procDecl a_procDecl_Func(int pos, a_name name, a_formals formals,
        a_varDecls varDecls, a_stmt stmt) {
    a_procDecl p = chkalloc(sizeof(*p));
    p->type = t_procDecl_func;
    p->pos = pos;
    p->name = name;
    p->formals = formals;
    p->varDecls = varDecls;
    p->stmt = stmt;
    return p;
}

//========================================================================
// Formal parameters 
//========================================================================

// Formals
a_formals a_Formals(t_formal type, a_paramDeclSeq params, a_formals next) {
    a_formals p = chkalloc(sizeof(*p));
    p->type = type;
    p->params = params;
    p->next = next;
    return p;
}

// ParamDeclSeq
a_paramDeclSeq a_ParamDeclSeq(a_varId param, a_paramDeclSeq next) {
    a_paramDeclSeq p = chkalloc(sizeof(*p));
    p->param = param;
    p->next = next;
    return p;
}

//========================================================================
// Statements 
//========================================================================

// stmtSeq
a_stmtSeq a_StmtSeq(a_stmt head, a_stmtSeq tail) {
    a_stmtSeq p = chkalloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

// stmtPar
a_stmtPar a_StmtPar(a_stmt head, a_stmtPar tail) {
    a_stmtPar p = chkalloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

// stmt_skip
a_stmt a_stmt_Skip(int pos) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_skip;
    p->pos = pos;
    return p;
}

// stmt_return
a_stmt a_stmt_Return(int pos, a_expr expr) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_return;
    p->pos = pos;
    p->u.return_.expr = expr;
    return p;
}

// stmt_if
a_stmt a_stmt_If(int pos, a_expr expr, a_stmt stmt1, a_stmt stmt2) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_if;
    p->pos = pos;
    p->u.if_.expr = expr;
    p->u.if_.stmt1 = stmt1;
    p->u.if_.stmt2 = stmt2;
    return p;
}

// stmt_while
a_stmt a_stmt_While(int pos, a_expr expr, a_stmt stmt) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_while;
    p->pos = pos;
    p->u.while_.expr = expr;
    p->u.while_.stmt = stmt;
    return p;
}

// stmt_For
a_stmt a_stmt_For(int pos, a_elem var, a_expr pre, a_expr post, a_stmt stmt) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_for;
    p->pos = pos;
    p->u.for_.var = var;
    p->u.for_.pre = pre;
    p->u.for_.post = post;
    p->u.for_.stmt = stmt;
    return p;
}

// stmt_pCall
a_stmt a_stmt_PCall(int pos, a_name name, a_exprList exprList) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_pCall;
    p->pos = pos;
    p->u.pCall.name = name;
    p->u.pCall.exprList = exprList;
    return p;
}

// stmt_name
a_stmt a_stmt_Ass(int pos, a_elem elem, a_expr expr) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_ass;
    p->pos = pos;
    p->u.ass.dst = elem;
    p->u.ass.src = expr;
    return p;
}

// stmt_input
a_stmt a_stmt_In(int pos, a_elem elem, a_expr expr) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_input;
    p->pos = pos;
    p->u.io.dst = elem;
    p->u.io.src = expr;
    return p;
}

// stmt_output
a_stmt a_stmt_Out(int pos, a_elem elem, a_expr expr) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_output;
    p->pos = pos;
    p->u.io.dst = elem;
    p->u.io.src = expr;
    return p;
}

// stmt_on
a_stmt a_stmt_On(int pos, a_elem dest, a_elem pCall) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_on;
    p->pos = pos;
    p->u.on.dest = dest;
    p->u.on.pCall = pCall;
    return p;
}

// stmt_alias
a_stmt a_stmt_Alias(int pos, a_elem dest, a_elem array, a_expr index) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_alias;
    p->pos = pos;
    p->u.alias.dst = dest;
    p->u.alias.array = array;
    p->u.alias.index = index;
    return p;
}

// stmt_connect
a_stmt a_stmt_Connect(int pos, a_elem to, a_elem c1, a_elem c2) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_connect;
    p->pos = pos;
    p->u.connect.to = to;
    p->u.connect.c1 = c1;
    p->u.connect.c2 = c2;
    return p;
}

// stmt_par
a_stmt a_stmt_Par(int pos, a_stmtPar list) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_par;
    p->pos = pos;
    p->u.par = list;
    return p;
}

// stmt_Seq
a_stmt a_stmt_Seq(int pos, a_stmtSeq list) {
    a_stmt p = chkalloc(sizeof(*p));
    p->type = t_stmt_seq;
    p->pos = pos;
    p->u.seq = list;
    return p;
}

//========================================================================
// Expressions 
//========================================================================

// ExprList
a_exprList a_Exprlist(a_expr head, a_exprList tail) {
    a_exprList p = chkalloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

// expr_*: none, neg, not
a_expr a_expr_monadic(int pos, t_expr type, a_elem elem) {
    a_expr p = chkalloc(sizeof(*p));
    p->type = type;
    p->pos = pos;
    p->u.monadic.elem = elem;
    return p;
}

// expr_*: plus, minus, mult, div, rem, or, and, xor, lshift, rshift, eq, ne,
// ls, le, gr, ge
a_expr a_expr_diadic(int pos, t_expr type, a_elem elem, a_expr expr) {
    a_expr p = chkalloc(sizeof(*p));
    p->type = type;
    p->pos = pos;
    p->u.diadic.elem = elem;
    p->u.diadic.expr = expr;
    return p;
}

//========================================================================
// Elements 
//========================================================================

// elem_name
a_elem a_elem_Name(int pos, a_name name) {
    a_elem p = chkalloc(sizeof(*p));
    p->type = t_elem_name;
    p->pos = pos;
    p->sym = NULL;
    p->u.name = name;
    return p;
}

// elem_pCall
a_elem a_elem_PCall(int pos, a_name name, a_exprList exprList) {
    a_elem p = chkalloc(sizeof(*p));
    p->type = t_elem_pCall;
    p->pos = pos;
    p->sym = NULL;
    p->u.call.name = name;
    p->u.call.exprList = exprList;
    return p;
}

// elem_fnCall
a_elem a_elem_FnCall(int pos, a_name name, a_exprList exprList) {
    a_elem p = chkalloc(sizeof(*p));
    p->type = t_elem_fCall;
    p->pos = pos;
    p->sym = NULL;
    p->u.call.name = name;
    p->u.call.exprList = exprList;
    return p;
}

// elem_number
a_elem a_elem_Number(int pos, int numval) {
    a_elem p = chkalloc(sizeof(*p));
    p->type = t_elem_number;
    p->pos = pos;
    p->sym = NULL;
    p->u.number = numval;
    return p;
}

// elem_boolean
a_elem a_elem_Boolean(int pos, bool boolval) {
    a_elem p = chkalloc(sizeof(*p));
    p->type = t_elem_boolean;
    p->pos = pos;
    p->sym = NULL;
    p->u.boolean = boolval;
    return p;
}

// elem_string
a_elem a_elem_String(int pos, string strval) {
    a_elem p = chkalloc(sizeof(*p));
    p->type = t_elem_string;
    p->pos = pos;
    p->sym = NULL;
    p->u.string.value = strval;
    return p;
}

// elem_expr
a_elem a_elem_Expr(int pos, a_expr expr) {
    a_elem p = chkalloc(sizeof(*p));
    p->type = t_elem_expr;
    p->pos = pos;
    p->sym = NULL;
    p->u.expr = expr;
    return p;
}

// elem_sub
a_elem a_elem_Sub(int pos, a_name name, a_expr expr) {
    a_elem p = chkalloc(sizeof(*p));
    p->type = t_elem_sub;
    p->pos = pos;
    p->sym = NULL;
    p->u.sub.name = name;
    p->u.sub.expr = expr;
    return p;
}

//========================================================================
// Name 
//========================================================================

// Name
a_name a_Name(int pos, string name) {
    a_name p = chkalloc(sizeof(*p));
    p->pos = pos;
    p->name = name;
    return p;
}

//========================================================================
// Strings 
//========================================================================

string a_varTypeStr(t_varDecl t) {
    return varTypeStr[t];
}

string a_formalTypeStr(t_formal t) {
    return formalTypeStr[t];
}

string a_elemTypeStr(a_elem e) {
    return elemTypeStr[e->type];
}
