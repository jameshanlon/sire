#include "statistics.h"

void stats_init() {
    stat_numProcedures    = 0;
    stat_numSymbols       = 0;
    stat_numKilledStmts   = 0;
    stat_numSpiltVars     = 0;
    stat_numInstructions  = 0;
    stat_numBlocksRemoved = 0;
}

void stats_dump(FILE *out) {
    printTitleRule(out, "Compilation statistics");
    fprintf(out, "  Symbols:              %d\n", stat_numSymbols);
    fprintf(out, "  Procedures/functions: %d\n", stat_numProcedures);
    fprintf(out, "  Basic blocks removed: %d\n", stat_numBlocksRemoved);
    fprintf(out, "  Killed statements:    %d\n", stat_numKilledStmts);
    fprintf(out, "  Spilt variables:      %d\n", stat_numSpiltVars);
    fprintf(out, "  Instructions:         %d\n", stat_numInstructions);
    printRule(out);
}
