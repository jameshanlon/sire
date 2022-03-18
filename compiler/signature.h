#ifndef SIGNATURE_H
#define SIGNATURE_H

#include "util.h"
#include "ast.h"
#include "symbol.h"

typedef struct signature_ *signature;
typedef struct sigTable_  *sigTable;

// sigTable methods
sigTable  sigTab_New(void);
signature sigTab_insert(sigTable, a_procDecl);
signature sigTab_lookup(sigTable, string);
bool      sigTab_checkArgs(sigTable, symTable, string, a_exprList);
int       sigTab_numArgs(sigTable, string);
void      sigTab_dump(sigTable, FILE *);

t_formal  sig_getFmlType(signature, int);

#endif
