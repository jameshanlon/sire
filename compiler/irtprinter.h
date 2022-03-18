#ifndef IRTPRINTER_H
#define IRTPRINTER_H

#include <stdio.h>
#include "ir.h"
#include "irt.h"

void print_proc(FILE *, ir_proc);
void p_stmt(FILE *, int, i_stmt);
string p_expr(i_expr);

#endif
