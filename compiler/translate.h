#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "list.h"
#include "ast.h"
#include "irt.h"
#include "frame.h"
#include "structures.h"

#define TRUE     0xFFFFFFFF
#define ALLONES  0xFFFFFFFF
#define FALSE    0
#define WORDSIZE 1 

void translate(structures);
int  trl_evalOp(t_expr type, int a, int b);
void dumpChildren(structures, FILE *);

#endif
