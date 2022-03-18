#include "instructions.h"

// Emit a section heading
void emit_sec(FILE *o, string title) {
    fprintf(o, "%s", ASM_COMMENT); printTitleRule(o, title);
}

// Emit an instruction
void emit(FILE *o, const string format, ...) {
    va_list ap;
    fprintf(o, "\t");
    va_start(ap, format);
    vfprintf(o, format, ap);
    va_end(ap);
    fprintf(o, "\n");
}

// 3 register
void emit_3r(FILE *o, t_inst mn, int op1, int op2, int op3) {
    switch(mn) {
    case i_ADD:  emit(o, "%-6s r%d, r%d, r%d", "add",  op1, op2, op3);  break; 
    case i_SUB:  emit(o, "%-6s r%d, r%d, r%d", "sub",  op1, op2, op3);  break; 
    case i_MUL:  emit(o, "%-6s r%d, r%d, r%d", "mul",  op1, op2, op3);  break; 
    case i_DIVS: emit(o, "%-6s r%d, r%d, r%d", "divs", op1, op2, op3);  break; 
    case i_REMS: emit(o, "%-6s r%d, r%d, r%d", "rems", op1, op2, op3);  break; 
    case i_OR:   emit(o, "%-6s r%d, r%d, r%d", "or",   op1, op2, op3);  break; 
    case i_AND:  emit(o, "%-6s r%d, r%d, r%d", "and",  op1, op2, op3);  break; 
    case i_XOR:  emit(o, "%-6s r%d, r%d, r%d", "xor",  op1, op2, op3);  break;  
    case i_SHL:  emit(o, "%-6s r%d, r%d, r%d", "shl",  op1, op2, op3);  break;  
    case i_SHR:  emit(o, "%-6s r%d, r%d, r%d", "shr",  op1, op2, op3);  break; 
    case i_EQ:   emit(o, "%-6s r%d, r%d, r%d", "eq",   op1, op2, op3);  break; 
    case i_LSS:  emit(o, "%-6s r%d, r%d, r%d", "lss",  op1, op2, op3);  break;  
    case i_LDW:  emit(o, "%-6s r%d, r%d[r%d]", "ldw",  op1, op2, op3);  break;  
    case i_STW:  emit(o, "%-6s r%d, r%d[r%d]", "stw",  op1, op2, op3);  break;
    case i_TSETR: emit(o, "%-6s t[r%d]:r%d, r%d", "set", op1, op2, op3); break;
    default: assert(0 && "invalid 3r instruction");
    }
}

// 2 register unsigned
void emit_2ru(FILE *o, t_inst mn, int op1, int op2, unsigned int imm) {
    switch(mn) {
    case i_ADDI:  emit(o, "%-6s r%d, r%d, %d", "add", op1, op2, imm); break; 
    case i_SUBI:  emit(o, "%-6s r%d, r%d, %d", "sub", op1, op2, imm); break; 
    case i_EQI:   emit(o, "%-6s r%d, r%d, %d", "eq",  op1, op2, imm); break; 
    case i_SHLI:  emit(o, "%-6s r%d, r%d, %d", "shl", op1, op2, imm); break; 
    case i_SHRI:  emit(o, "%-6s r%d, r%d, %d", "shr", op1, op2, imm); break;
    case i_LDAWF: emit(o, "%-6s r%d, r%d[%d]", "ldaw", op1, op2, imm); break;
    case i_STWI:  emit(o, "%-6s r%d, r%d[%d]", "stw", op1, op2, imm); break;
    case i_LDWI:
    default: assert(0 && "invalid 2ru instruction");
    }
}

// 2 register
void emit_2r(FILE *o, t_inst mn, int op1, int op2) {
    switch(mn) {
    case i_MOVE:    emit(o, "%-6s r%d, r%d",      "mov",    op1, op2); break;
    case i_IN:      emit(o, "%-6s r%d, res[r%d]", "in",     op1, op2); break;
    case i_OUT:     emit(o, "%-6s res[r%d], r%d", "out",    op1, op2); break;
    case i_GETST:   emit(o, "%-6s r%d, res[r%d]", "getst",  op2, op1); break; 
    case i_GETPS:   emit(o, "%-6s r%d, ps[r%d]",  "get",    op2, op1); break; 
    case i_SETPS:   emit(o, "%-6s ps[r%d], r%d",  "set",    op2, op1); break;
    case i_SETC:    emit(o, "%-6s res[r%d], r%d", "setc",   op2, op1); break;
    case i_SETCLK:  emit(o, "%-6s res[r%d], r%d", "setclk", op1, op2); break;
    case i_TINITPC: emit(o, "%-6s t[r%d]:pc, r%d", "init",  op2, op1); break;
    case i_TINITLR: emit(o, "%-6s t[r%d]:lr, r%d", "init",  op2, op1); break;
    case i_TINITSP: emit(o, "%-6s t[r%d]:sp, r%d", "init",  op2, op1); break; 
    case i_TINITDP: emit(o, "%-6s t[r%d]:dp, r%d", "init",  op2, op1); break; 
    case i_TINITCP: emit(o, "%-6s t[r%d]:cp, r%d", "init",  op2, op1); break; 
    default: assert(0 && "invalid 2r instruction");
    }
}

// 1 register unsigned (int)
void emit_1ru(FILE *o, t_inst mn, int op1, unsigned int imm) {
    emit_1rl(o, mn, op1, StringFmt("%d", imm));
}

// 1 register unsigned (string)
void emit_1rl(FILE *o, t_inst mn, int op1, string imm) {
    switch(mn) {
    case i_BF:     emit(o, "%-6s r%d, %s",      "bf",    op1, imm); break;
    case i_BT:     emit(o, "%-6s r%d, %s",      "bt",    op1, imm); break; 
    case i_GETR:   emit(o, "%-6s r%d, %s",      "getr",  op1, imm); break; 
    case i_LDC:    emit(o, "%-6s r%d, %s",      "ldc",   op1, imm); break;
    case i_LDAWCP: emit(o, "%-6s r%d, cp[%s]",  "ldaw",  op1, imm); break;
    case i_LDAWSP: emit(o, "%-6s r%d, sp[%s]",  "ldaw",  op1, imm); break;
    case i_LDAWDP: emit(o, "%-6s r%d, dp[%s]",  "ldaw",  op1, imm); break;
    case i_LDWCP:  emit(o, "%-6s r%d, cp[%s]",  "ldw",   op1, imm); break;
    case i_LDWDP:  emit(o, "%-6s r%d, dp[%s]",  "ldw",   op1, imm); break;
    case i_LDWSP:  emit(o, "%-6s r%d, sp[%s]",  "ldw",   op1, imm); break;
    case i_STWDP:  emit(o, "%-6s r%d, dp[%s]",  "stw",   op1, imm); break;
    case i_STWSP:  emit(o, "%-6s r%d, sp[%s]",  "stw",   op1, imm); break;
    case i_SETCI:  emit(o, "%-6s res[r%d], %s", "setc",  op1, imm); break;
    case i_OUTCT:  emit(o, "%-6s res[r%d], %s", "outct", op1, imm); break;
    case i_CHKCT:  emit(o, "%-6s res[r%d], %s", "chkct", op1, imm); break;
    default: assert(0 && "invalid 1ru instruction");
    }
}

// 1 register
void emit_1r(FILE *o, t_inst mn, int op1) {
    switch(mn) {
    case i_SETSP: emit(o, "%-6s sp, r%d",  "set",   op1); break; 
    case i_SETDP: emit(o, "%-6s dp, r%d",  "set",   op1); break; 
    case i_SETCP: emit(o, "%-6s cp, r%d",  "set",   op1); break; 
    case i_MSYNC: emit(o, "%-6s res[r%d]", "msync", op1); break; 
    case i_MJOIN: emit(o, "%-6s res[r%d]", "mjoin", op1); break; 
    case i_KCALL: emit(o, "%-6s r%d",      "kcall", op1); break; 
    default: assert(0 && "invalid 1r instruction");
    }
}

// Unsigned immediate value
void emit_u(FILE *o, t_inst mn, unsigned int imm) {
    emit_l(o, mn, StringFmt("%d", imm));
}

// Immediate label 
void emit_l(FILE *o, t_inst mn, string imm) {
    switch(mn) {
    case i_BL:     emit(o, "%-6s %s",          "bl",    imm); break;
    case i_BU:     emit(o, "%-6s %s",          "bu",    imm); break; 
    case i_ENTSP:  emit(o, "%-6s %s",          "entsp", imm); break; 
    case i_EXTSP:  emit(o, "%-6s %s",          "extsp", imm); break; 
    case i_RETSP:  emit(o, "%-6s %s",          "retsp", imm); break; 
    case i_LDAP:   emit(o, "%-6s r11, %s",     "ldap",  imm); break; 
    case i_LDAWCP: emit(o, "%-6s r11, cp[%s]", "ldaw",  imm); break; 
    case i_KCALLI: emit(o, "%-6s %s",          "kcall", imm); break; 
    case i_BLACP:  emit(o, "%-6s cp[%s]",      "bla",   imm); break;
    default: assert(0 && "invalid u instruction");
    }
}

// 0 register
void emit_0r(FILE *o, t_inst mn) {
    switch(mn) {
    case i_SSYNC:  emit(o, "%-6s", "ssync");  break; 
    case i_WAITEU: emit(o, "%-6s", "waiteu"); break; 
    default: assert(0 && "invalid 0r instruction");
    }
}

