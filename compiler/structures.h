#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "symbol.h"
#include "signature.h"
#include "label.h"
#include "ir.h"

typedef struct {
    symTable sym;
    sigTable sig;
    labelMap lbl;
    intRep   ir;
} *structures;

structures structs_New(void);

#endif
