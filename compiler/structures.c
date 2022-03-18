#include <stdlib.h>
#include "structures.h"

// An object to store references to compiler data structures, global to the
// program.
structures structs_New(void) {
    structures p = (structures) chkalloc(sizeof(*p));
    p->sym = symTab_New();
    p->sig = sigTab_New();
    p->lbl = lblMap_New();
    p->ir  = ir_New();
    return p;
}
