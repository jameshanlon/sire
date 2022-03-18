#ifndef LABEL_H
#define LABEL_H

#include <stdio.h>
#include "util.h"

typedef struct labelMap_  *labelMap;
typedef struct label_     *label;

// LabelMap methods
labelMap  lblMap_New(void);
label     lblMap_NewLabel(labelMap);
label     lblMap_NewNamedLabel(labelMap, string);
label     lblMap_getNamed(labelMap, string);
void      lblMap_delete(labelMap);
void      lblMap_dump(labelMap, FILE *out);

// Label methods
string    lbl_name(label);
bool      lbl_compare(label, label);
void      lbl_setPos(label, int);
int       lbl_pos(label);
void      lbl_rename(label, string);

#endif
