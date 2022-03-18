#ifndef STATISTIC_H
#define STATISTIC_H

#include <stdio.h>
#include "util.h"

int stat_numProcedures;
int stat_numSymbols;
int stat_numKilledStmts;
int stat_numSpiltVars;
int stat_numInstructions;
int stat_numBlocksRemoved;

void stats_init(void);
void stats_dump(FILE *);

#endif
