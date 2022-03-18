#ifndef BLOCK_H
#define BLOCK_H

#include <stdio.h>
#include "util.h"
#include "list.h"
#include "frame.h"
#include "structures.h"

typedef struct block_ *block;

void   basicBlocks(structures);
list   blc_stmtSeq(list);
void   blc_labelStmts(list);
void   blc_dump(FILE *, list);

// Block object methods
string blc_name(block b);
list   blc_stmts(block b);
block  blc_getSucc1(block);
block  blc_getSucc2(block);

#endif
