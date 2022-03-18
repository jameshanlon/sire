#ifndef FRAME_H
#define FRAME_H

#include <stdio.h>
#include "util.h"
#include "list.h"
#include "ast.h"
#include "irt.h"
#include "label.h"
#include "temp.h"

#define NUM_PARAM_REGS 4  // r0-r3
#define NUM_GPRS       11 // r0-r10
                          // r11?

typedef enum {
    t_spill_none,
    t_spill_local,
    t_spill_caller,
    t_spill_global
} t_spill;

typedef struct frame_      *frame;
typedef struct frm_access_ *frm_access;
typedef struct frm_formal_ *frm_formal;

// Variables
frame      frm_New(label, list formals);
frm_formal frm_Formal(string, t_formal);
int        frm_allocArray(frame, string, int);
int        frm_allocLocal(frame, string);
void       frm_pCallArgs(frame, int);
//void       frm_migrateArgs(frame, int);
int        frm_arrayOffset(frame, string);

// Register allocation
temp       frm_addTemp(frame, string, t_temp);
temp       frm_addNewTemp(frame, t_temp type);
list       frm_regSet(frame);
list       frm_avalRegs(void);
t_spill    frm_spillType(frame, string);
int        frm_formalLocation(frame, string);
void       frm_resetStackOff(frame);
list       frm_formalAccesses(frame);
int        frm_allocReg(frame, list);
void       frm_setUsedRegs(frame, list);

// Calling convention
void       frm_genPrologue(frame, FILE *);
void       frm_genEpilogue(frame, FILE *);
void       frm_genCall(frame, i_expr, list, int, int, FILE *);

// Offsets
int        frm_size(frame);
int        frm_outArgOff(frame);
int        frm_preservedOff(frame);
int        frm_arrayOff(frame);
int        frm_localOff(frame);
int        frm_inArgOff(frame);
int        frm_numOutArgs(frame);

// Printing
string     frm_name(frame);
void       frm_rename(frame, string);
string     frm_formalStr(frame);
void       frm_dump(FILE *, frame);

// Frame access object methods
string     frm_access_name(frm_access);
int        frm_access_reg(frm_access);
int        frm_access_off(frm_access);
bool       frm_access_inReg(frm_access);
bool       frm_access_inFrame(frm_access);
void       frm_setEpilogueLbl(frame, label);
label      frm_getEpilogueLbl(frame);

// Reg object methods
int       *reg_Reg(int num);
bool       reg_cmp(void *, void *);
string     reg_show(void *);
void       reg_delete(void *reg);

#endif
