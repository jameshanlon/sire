#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdarg.h>
#include "list.h"
#include "../include/definitions.h"
#include "frame.h"
#include "structures.h"

//#define PROC_MAIN "_main"
//#define INIT_FUNC "_start"

#define RETURN_REG     0
#define REG_GDEST      11

void   gen_program     (FILE *, FILE *, FILE *, structures);
string gen_procLabelStr(string);
bool   gen_inImmRangeS (int);
bool   gen_inImmRangeL (int);

#endif
