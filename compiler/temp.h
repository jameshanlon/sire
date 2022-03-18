#ifndef TEMP_H
#define TEMP_H

#include <stdio.h>
#include "util.h"

typedef struct tempMap_ *tempMap;
typedef struct temp_    *temp;

typedef enum {
    t_tmp_global,
    t_tmp_local
} t_temp;

typedef enum {
    t_tmpAccess_undefined,
    t_tmpAccess_reg,
    t_tmpAccess_frame,
    t_tmpAccess_caller,
    t_tmpAccess_data,
    t_tmpAccess_none
} t_tmpAccess;

tempMap     tmp_New(void);
temp        tmp_Temp(tempMap, string, t_temp);
string      tmp_NewName(tempMap);
temp        tmp_lookup(tempMap, string);
string      tmp_name(temp);
t_temp      tmp_type(temp);

void        tmp_setRegAccess(temp, int);
void        tmp_setFrameAccess(temp, int);
void        tmp_setCallerAccess(temp, int);
void        tmp_setDataAccess(temp); 
void        tmp_setAccessNone(temp);
void        tmp_setAccessUnassigned(temp);
t_tmpAccess tmp_getAccess(temp);
int         tmp_reg(temp);
int         tmp_off(temp);
void        tmp_setSpilled(temp, string);
bool        tmp_spill(temp);
string      tmp_spilled(temp);

string      tmp_str(void *);
bool        tmp_cmpName(void *t, void *name);
bool        tmp_cmpTemp(void *, void *);

#endif
