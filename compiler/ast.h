#ifndef AST_H
#define AST_H

#include "util.h"
#include "label.h"
#include "symbol.h"

// Abstract syntax nodes
typedef struct _a_module       *a_module;
typedef struct _a_constDecls   *a_constDecls;
typedef struct _a_constDecl    *a_constDecl;
typedef struct _a_portDecls    *a_portDecls;
typedef struct _a_portDecl     *a_portDecl;
typedef struct _a_varDecls     *a_varDecls;
typedef struct _a_varDecl      *a_varDecl;
typedef struct _a_varType      *a_varType;
typedef struct _a_idList       *a_idList;
typedef struct _a_varId        *a_varId;
typedef struct _a_procDecls    *a_procDecls;
typedef struct _a_procDecl     *a_procDecl;
typedef struct _a_formals      *a_formals;
typedef struct _a_paramDeclSeq *a_paramDeclSeq;
typedef struct _a_formal       *a_formal;
typedef struct _a_elem         *a_elem;
typedef struct _a_exprList     *a_exprList;
typedef struct _a_expr         *a_expr;
typedef struct _a_stmtSeq      *a_stmtSeq;
typedef struct _a_stmtPar      *a_stmtPar;
typedef struct _a_stmt         *a_stmt;
typedef struct _a_name         *a_name;

// Variable declaration types
typedef enum {
    t_varDecl_chan, 
    t_varDecl_int, 
    t_varDecl_chanArray,
    t_varDecl_intArray,
    t_varDecl_chanAlias,
    t_varDecl_intAlias
} t_varDecl;

// Formal argument types
typedef enum {
    t_formal_int,
    t_formal_port,
    t_formal_chanend,
    t_formal_intArray,
    t_formal_chanArray,
} t_formal;

// Expression operators
typedef enum {
    t_expr_none, // when just (elem)
    t_expr_neg,
    t_expr_not,
    t_expr_plus,
    t_expr_minus,
    t_expr_mult,
    t_expr_div,
    t_expr_rem,
    t_expr_or,
    t_expr_and,
    t_expr_xor,
    t_expr_lshift,
    t_expr_rshift,
    t_expr_eq,
    t_expr_ne,
    t_expr_ls,
    t_expr_le,
    t_expr_gr,
    t_expr_ge
} t_expr;

// Module
struct _a_module {
    enum {
        t_module_main, 
        t_module_def
    } type;
    int pos;
    a_constDecls consts; 
    a_portDecls ports; 
    a_varDecls vars;
    a_procDecls procs;
    //a_name name; 
    //a_formals formals; 
};

// ConstDecls
struct _a_constDecls {
    a_constDecl decl;
    a_constDecls next;
};

// ConstDecl
struct _a_constDecl {
    int pos;
    a_name name;
    a_expr expr;
    int value;
};

// PortDecls
struct _a_portDecls {
    a_portDecl decl;
    a_portDecls next;
};

// PortDecl
struct _a_portDecl {
    int pos;
    a_name name;
    a_expr expr;
    int num;
};

// VarDecls
struct _a_varDecls {
    a_varDecl  vars;
    a_varDecls next;
};

// VarDecl
struct _a_varDecl {
    int pos;
    a_idList ids;
    a_varType type;
};

// VarType
struct _a_varType {
    int pos;
    t_varDecl type;
    a_expr expr;
    int len;
};

// IdList
struct _a_idList {
    a_varId var;
    a_idList next;
};

// VarId
struct _a_varId {
    int pos;
    a_name name;
    a_expr expr;
};

// ProcDecls
struct _a_procDecls {
    a_procDecl head;
    a_procDecls tail;
};

// Procdecl
struct _a_procDecl {
    enum {
        t_procDecl_proc, 
        t_procDecl_func
    } type;
    int pos;
    a_name name;
    a_formals formals;
    a_varDecls varDecls;
    a_stmt stmt;
};

// Formals
struct _a_formals {
    t_formal type;
    a_paramDeclSeq params;
    a_formals next;
};

// ParamDeclSeq
struct _a_paramDeclSeq {
    a_varId param;
    a_paramDeclSeq next;
};

// StmtSeq
struct _a_stmtSeq {
    a_stmt head;
    a_stmtSeq tail;
};

// StmtPar
struct _a_stmtPar {
    a_stmt head;
    a_stmtPar tail;
};

// Stmt
struct _a_stmt {
    enum t_stmt {
        t_stmt_skip,
        t_stmt_return,
        t_stmt_when,
        t_stmt_if,
        t_stmt_while,
        t_stmt_for,
        t_stmt_pCall,
        t_stmt_ass,
        t_stmt_input,
        t_stmt_output,
        t_stmt_on,
        t_stmt_alias,
        t_stmt_connect,
        t_stmt_seq,
        t_stmt_par
    } type;
    int pos;
    union {
        struct {
            a_expr expr;
        } return_;
        struct {
            a_expr expr;
            a_stmt stmt1;
            a_stmt stmt2;
        } if_;
        struct {
            a_expr expr;
            a_stmt stmt;
        } while_;
        struct {
            a_elem var;
            a_expr pre;
            a_expr post;
            a_stmt stmt;
        } for_;
        struct {
            a_name name;
            a_exprList exprList;
        } pCall;
        struct {
            a_elem dst; // (left)
            a_expr src;
        } ass;
        struct {
            a_elem dst; // (left)
            a_expr src;
        } io;
        struct {
            a_elem dest;
            a_elem pCall;
        } on;
        struct {
            a_elem dst;
            a_elem array;
            a_expr index;
        } alias;
        struct {
            a_elem to;     // core[dest]
            a_elem c1, c2; // chan[]
        } connect;
        a_stmtSeq seq;
        a_stmtPar par;
    } u;
};

// ExprList
struct _a_exprList {
    a_expr head;
    a_exprList tail;
};

// Expr
struct _a_expr {
    t_expr type;
    int pos;
    union {
        struct {
            a_elem elem;
        } monadic;
        struct {
            a_elem elem;
            a_expr expr;
        } diadic;
    } u;
};

// Element
struct _a_elem {
    enum t_elem {
        t_elem_name,
        t_elem_sub,
        t_elem_pCall, 
        t_elem_fCall, 
        t_elem_number,
        t_elem_boolean,
        t_elem_string,
        t_elem_expr
    } type;
    int pos;
    symbol sym;
    union {
        a_name name;
        int number;
        bool boolean;
        a_expr expr;
        struct {
            a_name name;
            a_exprList exprList;
        } call;
        struct {
           label l;
           string value;
        } string;
        struct {
            a_name name;
            a_expr expr;
        } sub;
    } u;
};

// Name
struct _a_name {
    int pos;
    string name;
};

// Program/module
a_module       a_module_Main   (int, 
        a_constDecls, a_portDecls, a_varDecls, a_procDecls);
       
// Constant declarations
a_constDecls   a_ConstDecls    (a_constDecl, a_constDecls);
a_constDecl    a_ConstDecl     (int, a_name, a_expr);

// Port declarations
a_portDecls    a_PortDecls     (a_portDecl,  a_portDecls);
a_portDecl     a_PortDecl      (int, a_name, a_expr);

// Variable declarations
a_varDecls     a_VarDecls      (a_varDecl, a_varDecls);
a_varDecl      a_VarDecl       (int, a_idList, a_varType);
a_varType      a_VarType       (int, t_varDecl, a_expr);
a_idList       a_IdList        (a_varId, a_idList);
a_varId        a_VarId         (int, a_name);

// Procedure declarations
a_procDecls    a_ProcDecls     (a_procDecl,  a_procDecls);
a_procDecl     a_procDecl_Proc (int, a_name, a_formals, a_varDecls, a_stmt);
a_procDecl     a_procDecl_Func (int, a_name, a_formals, a_varDecls, a_stmt);

// Formal parameters
a_formals      a_Formals       (t_formal, a_paramDeclSeq, a_formals);
a_paramDeclSeq a_ParamDeclSeq  (a_varId, a_paramDeclSeq);

// Statements
a_stmtSeq      a_StmtSeq       (a_stmt, a_stmtSeq);
a_stmtPar      a_StmtPar       (a_stmt, a_stmtPar);
a_stmt         a_stmt_Skip     (int);
a_stmt         a_stmt_Return   (int, a_expr expr);
a_stmt         a_stmt_If       (int, a_expr, a_stmt, a_stmt);
a_stmt         a_stmt_While    (int, a_expr, a_stmt);
a_stmt         a_stmt_For      (int, a_elem, a_expr, a_expr, a_stmt);
a_stmt         a_stmt_PCall    (int, a_name, a_exprList);
a_stmt         a_stmt_Ass      (int, a_elem, a_expr);
a_stmt         a_stmt_In       (int, a_elem, a_expr);
a_stmt         a_stmt_Out      (int, a_elem, a_expr);
a_stmt         a_stmt_On       (int, a_elem, a_elem);
a_stmt         a_stmt_Alias    (int, a_elem, a_elem, a_expr);
a_stmt         a_stmt_Connect  (int, a_elem, a_elem, a_elem);
a_stmt         a_stmt_Seq      (int, a_stmtSeq);
a_stmt         a_stmt_Par      (int, a_stmtPar);

// Expressions
a_exprList     a_Exprlist      (a_expr, a_exprList);
a_expr         a_expr_monadic  (int, t_expr, a_elem);
a_expr         a_expr_diadic   (int, t_expr, a_elem, a_expr);
              
// Elements
a_elem         a_elem_Name     (int, a_name);
a_elem         a_elem_Sub      (int, a_name, a_expr);
a_elem         a_elem_PCall    (int, a_name, a_exprList);
a_elem         a_elem_FnCall   (int, a_name, a_exprList);
a_elem         a_elem_Number   (int, int);
a_elem         a_elem_Boolean  (int, bool);
a_elem         a_elem_String   (int, string);
a_elem         a_elem_Expr     (int, a_expr);

// Name
a_name         a_Name          (int, string);

// Strings
string a_varTypeStr            (t_varDecl);
string a_formalTypeStr         (t_formal);
string a_elemTypeStr           (a_elem);

#endif
