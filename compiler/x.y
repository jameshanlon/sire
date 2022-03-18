%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "error.h"
#include "symbol.h"
#include "ast.h"

int yywrap(void) {
    cp = 1;
    return 1;
}

void yyerror(a_module *astRoot, char *s) {
    err_report(t_error, tp, s);
}

int yylex();

%}

/* Create a (pure) re-entrant parser (no globals) */
//%define api.pure full
%error-verbose
%parse-param {a_module *astRoot}

/* Operator presidence: lowest first, together equal */
%nonassoc EQ NE LS LE GR GE
%left LSHIFT RSHIFT AND OR XOR REM
%left PLUS MINUS
%left MULT DIV

%union{
    int              intval;
    unsigned int     uintval;
    bool             boolval;
    string           strval;
    a_module         module;
    a_constDecls     constDecls;
    a_constDecl      constDecl;
    a_portDecls      portDecls;
    a_portDecl       portDecl;
    a_varDecls       varDecls;
    a_varDecl        varDecl;
    a_idList         idList;
    a_varType        varType;
    a_varId          varId;
    a_procDecls      procDecls;
    a_procDecl       procDecl;
    a_formals        formals;
    a_paramDeclSeq   paramDeclSeq;
    t_formal         formalType;
    a_elem           elem;
    a_exprList       exprList;
    a_expr           expr;
    a_stmtSeq        stmtSeq;
    a_stmtPar        stmtPar;
    a_stmt           stmt;
    a_name           name;
}

%token <uintval> NUMBER      
%token <strval>  ID STRING
%token <boolval> TRUE FALSE       
%token LBRACKET RBRACKET LPAREN RPAREN 
%token PROC FUNC IS BODY RETURN 
%token IF THEN ELSE WHILE DO TO FOR ON SKP ALIASES DOTS CONNECT CORE
%token ASS INPUT OUTPUT 
%token START END 
%token SEMICOLON BAR COMMA COLON
%token VAR CHAN CHANEND PORT TIMER INT CONST
%token MASTER SLAVE
%token PLUS MINUS MULT DIV REM OR AND XOR LSHIFT RSHIFT 
    EQ NE LS LE GR GE NOT NEG

%type <module>       program
%type <constDecls>   const_decls const_decl_seq
%type <constDecl>    const_decl
%type <portDecls>    port_decls port_decl_seq
%type <portDecl>     port_decl
%type <varDecls>     var_decls var_decls_seq
%type <varDecl>      var_decl
%type <varType>      var_type
%type <idList>       id_list
%type <varId>        var_id
%type <procDecls>    proc_decls proc_decl_seq 
%type <procDecl>     proc_decl
%type <formals>      formals formals_seq
%type <paramDeclSeq> param_decl_seq
%type <formalType>   param_type
%type <stmtSeq>      stmt_seq
%type <stmtPar>      stmt_par
%type <stmt>         stmt proc_call
%type <exprList>     expr_list
%type <expr>         expr right
%type <elem>         elem left on_proc_call name_elem
%type <name>         name

%start program

%%

//=============================================================================
// Program declaration
//=============================================================================

program :     
    const_decls port_decls var_decls proc_decls 
    { *astRoot = a_module_Main(tp, $1, $2, $3, $4); }
  ;

//=============================================================================
// Constant declarations
//=============================================================================

const_decls :
    /* empty */                             { $$ = NULL; }
  | CONST const_decl_seq                    { $$ = $2;   }
  ;

const_decl_seq :
    const_decl                              { $$ = a_ConstDecls($1, NULL); }
  | const_decl COMMA const_decl_seq         { $$ = a_ConstDecls($1, $3);   }
  ;

const_decl :
    name ASS expr                           { $$ = a_ConstDecl(tp, $1, $3);    }
  ;

//=============================================================================
// Port declarations
//=============================================================================

port_decls :
    /* empty */                             { $$ = NULL; }
  | PORT port_decl_seq                      { $$ = $2;   }
  ;

port_decl_seq :
    port_decl                               { $$ = a_PortDecls($1, NULL); }
  | port_decl COMMA port_decl_seq           { $$ = a_PortDecls($1, $3);   }
  ;

port_decl :
    name COLON expr                         { $$ = a_PortDecl(tp, $1, $3);  }
  ;

//=============================================================================
// Variable declarations
//=============================================================================

var_decls : 
  /* empty */                     { $$ = NULL; }
  | VAR var_decls_seq             { $$ = $2; }
  ;

var_decls_seq :
    var_decl                      { $$ = a_VarDecls($1, NULL); }
  | var_decl SEMICOLON var_decls_seq { $$ = a_VarDecls($1, $3); }
  | error SEMICOLON               { yyerrok; }
  ;

var_decl:
    id_list COLON var_type        { $$ = a_VarDecl(tp, $1, $3); }
  ;

id_list :
    var_id                        { $$ = a_IdList($1, NULL); }
  | var_id COMMA id_list          { $$ = a_IdList($1, $3); }
  ;

var_type :
    INT                           { $$ = a_VarType(tp, t_varDecl_int, NULL); }
  /*| CHAN                          { $$ = a_VarType(tp, t_varDecl_chan, NULL); }*/
  | INT LBRACKET RBRACKET         { $$ = a_VarType(tp, t_varDecl_intAlias, NULL); }
  | INT LBRACKET expr RBRACKET    { $$ = a_VarType(tp, t_varDecl_intArray, $3); }
  ;

var_id :
    name                          { $$ = a_VarId(tp, $1); }
  ;

//=============================================================================
// Procedure declarations
//=============================================================================

proc_decls : 
  /* empty */                               { $$ = NULL; }
  | proc_decl_seq                           { $$ = $1; }
  ;

proc_decl_seq :
    proc_decl                               { $$ = a_ProcDecls($1, NULL); }
  | proc_decl proc_decl_seq                 { $$ = a_ProcDecls($1, $2); }
  ;

proc_decl : 
     PROC name LPAREN formals RPAREN IS var_decls stmt 
                             { $$ = a_procDecl_Proc(tp, $2, $4, $7, $8); } 
  |  FUNC name LPAREN formals RPAREN IS var_decls stmt 
                             { $$ = a_procDecl_Func(tp, $2, $4, $7, $8); }
  ;

//=============================================================================
// Formal parameter declarations
//=============================================================================

formals : 
  /* empty */                               { $$ = NULL; }
  | formals_seq                             { $$ = $1; }
  ;

formals_seq :
    param_decl_seq COLON param_type                         
                                            { $$ = a_Formals($3, $1, NULL); }
  | param_decl_seq COLON param_type SEMICOLON formals_seq   
                                            { $$ = a_Formals($3, $1, $5); }
  ;                                        
                                           
param_type :                              
    INT                                     { $$ = t_formal_int; }
  /*| PORT                                    { $$ = t_formal_port; }
  | CHANEND                                 { $$ = t_formal_chanend; }*/
  | INT LBRACKET RBRACKET                   { $$ = t_formal_intArray; }
  ;

param_decl_seq :
    var_id                               { $$ = a_ParamDeclSeq($1, NULL); }
  | var_id COMMA param_decl_seq           { $$ = a_ParamDeclSeq($1, $3); }
  ;                                        

//=============================================================================
// Statements
//=============================================================================

stmt : 
    SKP                          { $$ = a_stmt_Skip    (tp);             }
  | proc_call                    { $$ = $1;                              }
  | left ASS expr                { $$ = a_stmt_Ass     (tp, $1, $3);     }
  | left INPUT expr              { $$ = a_stmt_In      (tp, $1, $3);     }
  | left OUTPUT expr             { $$ = a_stmt_Out     (tp, $1, $3);     }
  | IF expr THEN stmt ELSE stmt  { $$ = a_stmt_If      (tp, $2, $4, $6); }
  | WHILE expr DO stmt           { $$ = a_stmt_While   (tp, $2, $4);     }
  | FOR left ASS expr TO expr DO stmt     
                                 { $$ = a_stmt_For (tp, $2, $4, $6, $8); }
  | ON left COLON on_proc_call      
                                 { $$ = a_stmt_On      (tp, $2, $4);     } 
  | CONNECT left TO left COLON left
                                 { $$ = a_stmt_Connect (tp, $4, $2, $6); }
  | name_elem ALIASES name_elem LBRACKET expr DOTS RBRACKET
                                 { $$ = a_stmt_Alias   (tp, $1, $3, $5); }
  | RETURN expr                  { $$ = a_stmt_Return  (tp, $2);         }
  | START stmt_seq END           { $$ = a_stmt_Seq     (tp, $2);         }
  | START stmt_par END           { $$ = a_stmt_Par     (tp, $2);         }
  ;
  
stmt_seq : 
    stmt                         { $$ = a_StmtSeq($1, NULL);                 }  
  | stmt SEMICOLON stmt_seq      { $$ = a_StmtSeq($1, $3);                   }
  | error SEMICOLON stmt_seq     { yyerrok;                                  }
  ;
      
stmt_par : 
    stmt BAR stmt                { $$ = a_StmtPar($1, a_StmtPar($3, NULL));  } 
  | stmt BAR stmt_par            { $$ = a_StmtPar($1, $3);                   }
  | error BAR stmt_par           { yyerrok;                                  }
  ;
      
proc_call:
    name LPAREN RPAREN           { $$ = a_stmt_PCall(tp, $1, NULL);   }
  | name LPAREN expr_list RPAREN { $$ = a_stmt_PCall(tp, $1, $3);     }
  ;

on_proc_call:
    name LPAREN RPAREN           { $$ = a_elem_PCall(tp, $1, NULL);   }
  | name LPAREN expr_list RPAREN { $$ = a_elem_PCall(tp, $1, $3);     }
  ;

//=============================================================================
// Expressions 
//=============================================================================

expr_list : 
    expr                         { $$ = a_Exprlist    ($1, NULL);            }
  | expr COMMA expr_list         { $$ = a_Exprlist    ($1, $3);              }
  ;

expr :
    elem               { $$ = a_expr_monadic(tp, t_expr_none,   $1);     }
  | MINUS elem         { $$ = a_expr_monadic(tp, t_expr_neg,    $2);     }
  | NOT elem           { $$ = a_expr_monadic(tp, t_expr_not,    $2);     }
  | elem PLUS right    { $$ = a_expr_diadic (tp, t_expr_plus,   $1, $3); }
  | elem MINUS right   { $$ = a_expr_diadic (tp, t_expr_minus,  $1, $3); }
  | elem MULT right    { $$ = a_expr_diadic (tp, t_expr_mult,   $1, $3); }
  | elem DIV right     { $$ = a_expr_diadic (tp, t_expr_div,    $1, $3); }
  | elem REM right     { $$ = a_expr_diadic (tp, t_expr_rem,    $1, $3); }
  | elem OR right      { $$ = a_expr_diadic (tp, t_expr_or,     $1, $3); }
  | elem AND right     { $$ = a_expr_diadic (tp, t_expr_and,    $1, $3); }
  | elem XOR right     { $$ = a_expr_diadic (tp, t_expr_xor,    $1, $3); }
  | elem LSHIFT right  { $$ = a_expr_diadic (tp, t_expr_lshift, $1, $3); }
  | elem RSHIFT right  { $$ = a_expr_diadic (tp, t_expr_rshift, $1, $3); }
  | elem EQ right      { $$ = a_expr_diadic (tp, t_expr_eq,     $1, $3); }
  | elem NE right      { $$ = a_expr_diadic (tp, t_expr_ne,     $1, $3); }
  | elem LS right      { $$ = a_expr_diadic (tp, t_expr_ls,     $1, $3); }
  | elem LE right      { $$ = a_expr_diadic (tp, t_expr_le,     $1, $3); }
  | elem GR right      { $$ = a_expr_diadic (tp, t_expr_gr,     $1, $3); }
  | elem GE right      { $$ = a_expr_diadic (tp, t_expr_ge,     $1, $3); }
  ;

/* Associative operators */
right : 
    elem               { $$ = a_expr_monadic(tp, t_expr_none,   $1);     }
  | elem AND right     { $$ = a_expr_diadic (tp, t_expr_and,    $1, $3); }
  | elem OR right      { $$ = a_expr_diadic (tp, t_expr_or,     $1, $3); }
  | elem XOR right     { $$ = a_expr_diadic (tp, t_expr_xor,    $1, $3); }
  | elem PLUS right    { $$ = a_expr_diadic (tp, t_expr_plus,   $1, $3); }
  | elem MULT right    { $$ = a_expr_diadic (tp, t_expr_mult,   $1, $3); }
  ;

//=============================================================================
// Elements 
//=============================================================================

left : 
    name                         { $$ = a_elem_Name    (tp, $1);         }
  | name LBRACKET expr RBRACKET  { $$ = a_elem_Sub     (tp, $1, $3);     }
  ;

elem :
    name                         { $$ = a_elem_Name    (tp, $1);         }
  | name LBRACKET expr RBRACKET  { $$ = a_elem_Sub     (tp, $1, $3);     }
  | name LPAREN RPAREN           { $$ = a_elem_FnCall  (tp, $1, NULL);   }
  | name LPAREN expr_list RPAREN { $$ = a_elem_FnCall  (tp, $1, $3);     }
  | NUMBER                       { $$ = a_elem_Number  (tp, $1);         }
  | TRUE                         { $$ = a_elem_Boolean (tp, $1);         }
  | FALSE                        { $$ = a_elem_Boolean (tp, $1);         }
  | STRING                       { $$ = a_elem_String  (tp, $1);         }
  | LPAREN expr RPAREN           { $$ = a_elem_Expr    (tp, $2);         }
  ;

name_elem :
    name                         { $$ = a_elem_Name(tp, $1);             }

name : 
    ID                           { $$ = a_Name(tp, $1);                  }
  ;

%%

