#include <stdio.h>
#include <stdarg.h>
#include "util.h"

#define ASM_COMMENT "//"

typedef enum {
    
    // 3r
    i_ADD,
    i_SUB,
    i_MUL,
    i_DIVS,
    i_REMS,
    i_OR,
    i_AND,
    i_XOR,
    i_SHL,
    i_SHR,
    i_EQ,
    i_LSS,
    i_LDW,
    i_STW,
    i_TSETR,

    // 2r
    i_IN,
    i_OUT,
    i_MOVE,
    i_GETST,
    i_GETPS,
    i_SETPS,
    i_SETC,
    i_SETCLK,
    i_TINITPC,
    i_TINITLR,
    i_TINITSP,
    i_TINITCP,
    i_TINITDP,

    // 2ru
    i_ADDI,
    i_SUBI,
    i_EQI,
    i_SHLI,
    i_SHRI,
    i_LDWI,
    i_STWI,
    i_LDAWF,

    // 1ru
    i_BF,
    i_BT,
    i_GETR,
    i_LDC,
    i_LDAWSP,
    i_LDAWDP,
    i_LDWCP,
    i_LDWDP,
    i_LDWSP,
    i_SETCI,
    i_STWDP,
    i_STWSP,
    i_CHKCT,
    i_OUTCT,

    // 1r
    i_SETSP,
    i_SETDP,
    i_SETCP,
    i_MSYNC,
    i_MJOIN,
    i_KCALL,

    // u
    i_ENTSP,
    i_EXTSP,
    i_RETSP,
    i_LDAWCP,
    i_LDAP,
    i_BL,
    i_BU,
    i_KCALLI,
    i_BLACP,

    // 0r
    i_SSYNC,
    i_WAITEU
} t_inst;

void emit_3r  (FILE *, t_inst, int, int, int);
void emit_2r  (FILE *, t_inst, int, int);
void emit_2ru (FILE *, t_inst, int, int, unsigned int);
void emit_1r  (FILE *, t_inst, int);
void emit_1ru (FILE *, t_inst, int, unsigned int);
void emit_1rl (FILE *, t_inst, int, string);
void emit_0r  (FILE *, t_inst);
void emit_u   (FILE *, t_inst, unsigned int);
void emit_l   (FILE *, t_inst, string);

void emit_sec (FILE *, string);
void emit     (FILE *, const string, ...);

