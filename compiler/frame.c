#include <stdlib.h>
#include <string.h>
#include "frame.h"
#include "codegen.h"
#include "instructions.h"

// Register usage types
typedef enum {
    t_regUsage_unused,
    t_regUsage_param,
    t_regUsage_scratch
} regUsageType;

static char regUsageStr[][8] = {
    "unused",
    "param",
    "scratch"
};

// Frame data structure
struct frame_ {
    label name;
    label epilogue;
    int numUsedParamRegs;
    int branchLink;
    int maxOutArgs;
    int numOutArgs;
    int arraySpace;
    int localSpace;
    int numPreserved;
    int inArgOffset;
    tempMap temps;
    regUsageType regUsage[NUM_GPRS];
    list formals;
    list locals;
    list preserved;
};

typedef enum {
    t_inReg,
    t_inFrame
} t_accessLocation;

typedef enum {
    // Formal accesses
    t_access_f_int,
    t_access_f_port,
    t_access_f_chanend,
    t_access_f_intArray,
    // Stack accesses
    t_access_array,
    t_access_local,
    t_access_preserved
} t_accessType;

static char accessTypeStr[][10] = {
    "val",
    "port",
    "chanend",
    "var",
    "array",
    "array",
    "local",
    "preserved"
};

// Frame access data structure
struct frm_access_ {
    t_accessType type;
    t_accessLocation location;
    string name;
    union {
        int offset; // frame offset
        int reg;    // register
    } u;
};

// A formal parameter
struct frm_formal_ {
    t_formal type;
    string name;
};

// Prototypes
static frm_access   InFrame(t_accessType, string, int);
static frm_access   InReg(t_accessType, string, int);
static bool         cmpAccessName(void *, void *);
static string       savedRegStr(int);
static void         preserveParamRegs(frame, FILE *, int, bool);
static void         initArgs(FILE *, list);
static string       accessStr(frm_access);
static t_accessType getFormalAccessType(t_formal);

// Constructor
frame frm_New(label name, list refs) {
    
    frame f = (frame) chkalloc(sizeof(*f));
    f->name = name;
    f->branchLink = 0;
    f->maxOutArgs = 0;
    f->numOutArgs = 0;
    f->numUsedParamRegs = 0;
    f->inArgOffset = 0;
    f->temps = tmp_New();
    f->formals = list_New();
    f->locals = list_New();
    f->preserved = list_New();

    // Initialise a register usage array
    int i;
    for(i=0; i<NUM_GPRS; i++)
        f->regUsage[i] = t_regUsage_unused;

    // Construct access list for formal parameters
    iterator it = it_begin(refs);
    while(it_hasNext(it)) {
        frm_formal formal = it_next(it);
        
        // If there is an available register
        if(f->numUsedParamRegs < NUM_PARAM_REGS) {
            list_add(f->formals, InReg(getFormalAccessType(formal->type), 
                        formal->name, f->numUsedParamRegs));
            f->regUsage[f->numUsedParamRegs] = t_regUsage_param;
            f->numUsedParamRegs++;
            //printf("added formal access reg for %s\n", lbl_name(name));
        }
        // Otherwise it will be passed on the stack
        else {
            list_add(f->formals, InFrame(getFormalAccessType(formal->type), 
                        formal->name, f->inArgOffset));
            f->inArgOffset++;
            //printf("added formal access stack for %s\n", lbl_name(name));
        }
    }
    it_free(&it);
    
    /*printf("\nFormal accesses for %s:\n", lbl_name(name));
    it = it_begin(f->formals);
    while(it_hasNext(it)) {
        frm_access a = it_next(it);
        printf("  %s\n", accessStr(a));
    }
    it_free(&it);*/

    return f;
}

// For a variable n, to be spilled in memory, return the type of the location.
// This can be either as a local in the current stack frame, as an outgoing
// parameter of the previous stack frame, or as a global value in the data pool.
t_spill frm_spillType(frame f, string name) {
    
    // Check if it's a formal with allocated memory in the stack
    frm_access a = list_getFirst(f->formals, name, &cmpAccessName);
    if(a != NULL) {
        if(a->location == t_inFrame)
            return t_spill_caller;
    }
    
    // Otherwise check if it's a local
    temp t = tmp_lookup(f->temps, name);
    if(tmp_type(t) == t_tmp_local)
        return t_spill_local;
    
    return t_spill_global;
}

// Return the location of the formal parameter in the callers frame as the
// offset from the end of the current frame.
int frm_formalLocation(frame f, string name) {
    frm_access a = list_getFirst(f->formals, name, &cmpAccessName);
    assert(a != NULL && a->location != t_inReg 
            && "Invalid frm_access for formal variable");
    return a->u.offset;
}

// Allocate an array
int frm_allocArray(frame f, string name, int size) {
    //printf("allocated local %s in frame %s\n",  name, frm_name(f));
    frm_access a = InFrame(t_access_array, name, f->arraySpace);
    list_add(f->locals, a);
    f->arraySpace += size;
    return a->u.offset;
}

// Allocate a local variable in the frame, if it's not a global
int frm_allocLocal(frame f, string name) {
    //printf("allocated local %s in frame %s\n",  name, frm_name(f));
    frm_access a = InFrame(t_access_local, name, 
            f->arraySpace + f->localSpace);
    list_add(f->locals, a);
    f->localSpace++;
    return a->u.offset;
}

// Allocate space for a perserved register
static int frm_allocPreserved(frame f, int reg) {
    //printf("allocating preserved r%d in %s\n", reg, frm_name(f));
    frm_access a = InFrame(t_access_preserved, savedRegStr(reg), 
            f->numPreserved);
    list_add(f->preserved, a);
    f->numPreserved++;
    return a->u.offset;
}

// Notify the frame a procedure call is made with numArgs so that it can
// allocate stack space if necessary
void frm_pCallArgs(frame f, int num) {
    if(num > f->maxOutArgs) {
        f->maxOutArgs = num;
        f->numOutArgs = num > NUM_PARAM_REGS ? num - NUM_PARAM_REGS : 0;
    }
    f->branchLink = 1;
}

// Add space for migration call to kernel: outArgs needs to hold #args and each
// one
/*void frm_migrateArgs(frame f, int num) {
    if(num > f->maxOutArgs) {
        f->maxOutArgs = num;
        f->numOutArgs = num;
    }
}*/

// Variable access in frame. Formal offsets relative to sp[0+framesize]
// frame as size not yet known. Local offsets relative to sp[0]
static frm_access InFrame(t_accessType type, string name, int offset) {
    frm_access p = (frm_access) chkalloc(sizeof(*p));
    p->location = t_inFrame;
    p->type = type;
    p->name = name;
    p->u.offset = offset; 
    //printf("\tallocated frame%s at offset %d\n", ref?" REF":"", offset);
    return p;
}

// Variable access in a register (incoming arguments)
static frm_access InReg(t_accessType type, string name, int reg) {
    frm_access p = (frm_access) chkalloc(sizeof(*p));
    p->location = t_inReg;
    p->type = type;
    p->name = name;
    p->u.reg = reg;
    //printf("\tallocated reg%s in r%d\n", ref?" REF":"", reg);
    return p;
}

// Return the (local) offset of an array
int frm_arrayOffset(frame f, string name) {
    frm_access a = list_getFirst(f->locals, name, &cmpAccessName);
    assert(a != NULL && "local array does not exist");
    return a->u.offset;
}

// Return a list of colours (i.e. available general purpose registers to use),
// including any unused parameter registers r0-r3. Return them in ascending
// order to minimise number of preserves necessary around function calls.
list frm_regSet(frame f) {
    list regs = list_New();
    int i;
    for(i=NUM_GPRS-1; i>=f->numUsedParamRegs; i--)
        list_add(regs, reg_Reg(i));
    return regs;
}

// Allocate a non-peallocated variable a regsister not in the range 0 to
// f->numUsedParamRegs
int frm_allocReg(frame f, list regs) {
    iterator it = it_begin(regs);
    while(it_hasNext(it)) {
        int *reg = it_next(it);
        if(*reg >= f->numUsedParamRegs) {
            it_free(&it);
            return *reg;
        }
    }
    it_free(&it);
    return -1;
}

// Return a list of formal variables passed in registers
list frm_formalAccesses(frame f) {
    return f->formals;
}

// Add a new temp (relating to an existing variable name) to the map
temp frm_addTemp(frame f, string name, t_temp type) {
    return tmp_Temp(f->temps, name, type);
}

// Add a new temp, with a new variable name, to the map
temp frm_addNewTemp(frame f, t_temp type) {
    return tmp_Temp(f->temps, tmp_NewName(f->temps), type);
}

// Return the frame label
string frm_name(frame f) {
    return lbl_name(f->name);
}

// Create a frame formal reference flag
frm_formal frm_Formal(string name, t_formal type) {
    frm_formal f = (frm_formal) chkalloc(sizeof(*f));
    f->type = type;
    f->name = name;
    return f;
}

// Convert a t_formal -> t_accessType
t_accessType getFormalAccessType(t_formal t) {
    switch(t) {
    case t_formal_int:      return t_access_f_int;
    case t_formal_port:     return t_access_f_port;
    case t_formal_chanend:  return t_access_f_chanend;
    case t_formal_intArray: return t_access_f_intArray;
    default:
        assert(0 && "invalid formal access type");
        return -1;
    }
}

// Reset the stack offset after iterations of linear scan and program rewriting
void frm_resetStackOff(frame f) {
    f->localSpace = 0;
}

// Given a list of used registers (at end of regalloc phase), update regUsage
// array accordingly. After this call, we know how big the frame will be.
void frm_setUsedRegs(frame f, list usedRegs) {
    iterator it = it_begin(usedRegs);
    while(it_hasNext(it)) {
        int reg = *((int *) it_next(it));
        //printf("used reg %d\n", reg);

        // Set the usage to scratch if its not parameter reg
        if(reg >= f->numUsedParamRegs && reg < NUM_GPRS) {
            f->regUsage[reg] = t_regUsage_scratch;
            //printf("set regUsage to %d for reg %d\n", f->regUsage[*reg], *reg);
        }

        // Allocate stack space to save this register if between r4-r10
        if(reg >= NUM_PARAM_REGS && reg < NUM_GPRS)
            frm_allocPreserved(f, reg);

        // If we need to preserve any of r0-r3 
        if(reg < NUM_PARAM_REGS /*&& reg < f->maxOutArgs*/)
            frm_allocPreserved(f, reg);
    }
    it_free(&it);
}

// =======================================================================
// Calling convention
// =======================================================================

// Callee enter:
//  - "ENTSP n" (SP[0] = LR, SP = SP - n) this saves the LR in
//    SP[0] of caller frame, and extends the stack pointer by n words
//  - Push m required general purpose registers (r4-r10) onto stack from SP[0]
//    to SP[m] where m < 7
//  - Load m params passed on stack from SP[n+1] to SP[n+m]
void frm_genPrologue(frame f, FILE *out) {
    //fprintf(out, "; Procedure prologue: %s\n", frm_name(f));
    emit_u(out, i_ENTSP, frm_size(f));

    int i;
    for(i=NUM_PARAM_REGS; i<NUM_GPRS; i++) {
        if(f->regUsage[i] == t_regUsage_scratch) {
            frm_access a = list_getFirst(f->preserved, savedRegStr(i), &cmpAccessName);
            assert(a != NULL && "frm_access NULL for preserved GPR");
            int offset = a->u.offset + frm_preservedOff(f);
            emit_1ru(out, i_STWSP, i, offset);
        }
    }

    // TODO: load incoming stack arguments
}

// Callee exit:
//  - Pop m saved general purpose registers back from SP[0] to SP[m], to
//    r4-r(4+m)
//  - "RETSP n" (SP = SP + n, LR = SP[0], PC = LR)
void frm_genEpilogue(frame f, FILE *out) {
    //fprintf(out, "; Procedure epilogue: %s\n\n", frm_name(f));
    
    int i;
    for(i=NUM_PARAM_REGS; i<NUM_GPRS; i++) {
        if(f->regUsage[i] == t_regUsage_scratch) {
            frm_access a = list_getFirst(f->preserved, savedRegStr(i), &cmpAccessName);
            assert(a != NULL && "frm_access NULL for preserved GPR");
            int offset = a->u.offset + frm_preservedOff(f);
            emit_1ru(out, i_LDWSP, i, offset);
        }
    }
    
    emit_u(out, i_RETSP, frm_size(f));
}

// Store or load presevred parameter registers (r0-r3), excluding the
// destination register
// TODO: if a param reg is not used in the function body - i.e. it is not
// returned in the used set, then don't preserve it otherwise this assertion
// will trigger
static void preserveParamRegs(frame f, FILE *out, int dstReg, bool store) {
    int reg;
    for(reg=0; reg<NUM_PARAM_REGS; reg++) {
        if(f->regUsage[reg] != t_regUsage_unused) {
            frm_access a = list_getFirst(f->preserved, 
                    savedRegStr(reg), &cmpAccessName);
            //printf("preserving param reg %d\n", reg);
            assert(a != NULL && "frm_access NULL for parameter register");
            if(reg != dstReg) {
                int offset = frm_preservedOff(f) + a->u.offset;
                if(store) emit_1ru(out, i_STWSP, reg, offset);
                else      emit_1ru(out, i_LDWSP, reg, offset);
            }
        }
    }
}

// Initialise procedure call arguments
static void initArgs(FILE *out, list args) {
    
    int reg = 0;
    int argNum = 0;

    iterator it = it_begin(args);
    while(it_hasNext(it)) {
        i_expr arg = it_next(it);
        assert(arg->type == t_TEMP && "Call argument type not t_TEMP");
       
        // Move values in registers into correct regs or stack locaiton
        if(arg->type == t_TEMP) {
            temp t = arg->u.TEMP;
            if(reg < NUM_PARAM_REGS) {
                if(reg != tmp_reg(t))
                    emit_2r(out, i_MOVE, reg, tmp_reg(t));
                reg++;
            }
            else {
                int offset = argNum + 1;
                emit_1ru(out, i_STWSP, tmp_reg(t), offset);
                argNum++;
            }
        }
        // Otherwise load a constant
        else if(arg->type == t_CONST) {
            //int value = arg->u.CONST;
            //assert(gen_inImmRangeL(value));
            assert(0); // should not have const args?
        }
    }
    it_free(&it);
}

// Caller:
// - Put parameters (right-to-left order) in r0-r3, with additional m parameters
//   pushed on stack from sp[1] to sp[m+1]
// - Call function f with "bl f" (LR = pc+1)
// - Read return values 1-4 from r0-r3 and 5-r from SP[m+1] to SP[m+k]
void frm_genCall(frame f, i_expr proc, list args, int dstReg, 
        int callIndex, FILE *out) {

    emit(out, "%s begin call %s", ASM_COMMENT, lbl_name(proc->u.NAME));
    
    // Preserve param regs
    preserveParamRegs(f, out, dstReg, true);
    
    // Move parameter values into registers and stack locations
    initArgs(out, args);

    // Emit the branch and link instruction
    string name = lbl_name(proc->u.NAME); 
    //printf("/* Call: %s:%d */\n", lbl_name(proc->u.NAME), callIndex);
    emit_u(out, i_BLACP, callIndex+JUMP_INDEX_OFFSET); 
    //emit_l(out, i_BLACP, procJumpLblStr(name)); 

    // If its a function call, move result
    if(dstReg != -1 && dstReg != 0)
        emit_2r(out, i_MOVE, dstReg, 0);

    // Restore saved parameter registers
    preserveParamRegs(f, out, dstReg, false);

    emit(out, "%s end call %s", ASM_COMMENT, name);
}

// Frame offsets for memory accesses
int frm_size(frame f) {
    return f->branchLink 
        + f->numOutArgs
        + f->arraySpace 
        + f->localSpace 
        + f->numPreserved;
}

int frm_outArgOff(frame f) {
    return f->branchLink;
}

int frm_preservedOff(frame f) {
    return frm_outArgOff(f) + f->numOutArgs;
}

int frm_arrayOff(frame f) {
    return frm_preservedOff(f) + f->numPreserved; 
}

int frm_localOff(frame f) {
    return frm_arrayOff(f) + f->arraySpace; 
}

int frm_inArgOff(frame f) {
    return frm_localOff(f) + f->localSpace + 1;
}

int frm_numOutArgs(frame f) {
    return f->numOutArgs;
}

string frm_access_name(frm_access a) {
    return a->name;
}

void frm_setEpilogueLbl(frame f, label l) {
    f->epilogue = l;
}

label frm_getEpilogueLbl(frame f) {
    return f->epilogue;
}

void frm_rename(frame f, string name) {
    lbl_rename(f->name, name);
}

// =======================================================================
// Access data structure methods
// =======================================================================

int frm_access_reg(frm_access a) {
    assert(a->location == t_inReg && "frm_access not of type t_inReg");
    return a->u.reg;
}

int frm_access_off(frm_access a) {
    assert(a->location == t_inFrame && "frm_access not of type t_inFrame");
    return a->u.offset;
}

bool frm_access_inReg(frm_access a) {
    return a->location == t_inReg; 
}

bool frm_access_inFrame(frm_access a) {
    return a->location == t_inFrame; 
}

static bool cmpAccessName(void *formal_access, void *name) {
    return streq(((frm_access) formal_access)->name, name);
}

static string savedRegStr(int reg) {
    return StringFmt(".r%d_save", reg);
}

// Print out a single access
static string accessStr(frm_access a) {
    switch(a->location) {
    case t_inReg:   return StringFmt("%s: r%d", a->name, a->u.reg);
    case t_inFrame: return StringFmt("%s: sp[%d]", a->name, a->u.offset);
    default: assert(0 && "Invalid frm_access location");
    }
    return NULL;
}

// Return a string representation of the formal parameters for use in the IR
// printer
string frm_formalStr(frame f) {
    string s = "";
    iterator it = it_begin(f->formals);
    while(it_hasNext(it)) {
        frm_access a = it_next(it);
        string formal = StringFmt("%s %s%s", accessTypeStr[a->type], 
                a->name, it_hasNext(it) ? ", " : "");
        s = StringCat(s, formal);
        free(formal);
    }
    it_free(&it);
    return s;
}

// Dump the data for this frame
void frm_dump(FILE *out, frame f) {
    printTitleRule(out, StringFmt("Frame for %s", lbl_name(f->name)));
 
    fprintf(out, "\nRegister usages:\n");
    int reg;
    for(reg=0; reg<NUM_GPRS; reg++)
        printf("  reg %d usage: %s\n", reg, regUsageStr[f->regUsage[reg]]);

    fprintf(out, "\nOffsets:\n");
    fprintf(out, "  -----------------------\n");
    fprintf(out, "  Branch link:          %d\n", f->branchLink);
    fprintf(out, "  Outgoing args off:    %d\n", frm_outArgOff(f));
    fprintf(out, "  Preserved reg off:    %d\n", frm_preservedOff(f));
    fprintf(out, "  Local off: (array)    %d\n", frm_arrayOff(f));
    fprintf(out, "             (local)    %d\n", frm_localOff(f));
    fprintf(out, "  ----------------------- (%d)\n", frm_size(f));
    fprintf(out, "  Incoming arg off:     %d\n", frm_inArgOff(f));

    fprintf(out, "\nSizes:\n");
    fprintf(out, "  # used param regs:    %d\n", f->numUsedParamRegs);
    fprintf(out, "  # array space:        %d\n", f->arraySpace);
    fprintf(out, "  # local space:        %d\n", f->localSpace);
    fprintf(out, "  # preserved regs:     %d\n", f->numPreserved);
    fprintf(out, "  # outgoing arg space: %d\n", f->maxOutArgs);

    fprintf(out, "\nFormal accesses:\n");
    iterator it = it_begin(f->formals);
    while(it_hasNext(it)) {
        frm_access a = it_next(it);
        fprintf(out, "  %s\n", accessStr(a));
    }
    it_free(&it);
    
    fprintf(out, "\nLocal accesses:\n");
    it = it_begin(f->locals);
    while(it_hasNext(it)) {
        frm_access a = it_next(it);
        fprintf(out, "  %s\n", accessStr(a));
    }
    it_free(&it);

    fprintf(out, "\nPreserved accesses:\n");
    it = it_begin(f->preserved);
    while(it_hasNext(it)) {
        frm_access a = it_next(it);
        fprintf(out, "  %s\n", accessStr(a));
    }
    it_free(&it);

    fprintf(out, "\n");
    printRule(out);
}

// Reg value constructor
int *reg_Reg(int num) {
    int *r = chkalloc(sizeof(*r));
    *r = num;
    return r;
}

bool reg_cmp(void *r1, void *r2) { 
    return *((int *) r1) == *((int *) r2); 
}

string reg_show(void *reg) { 
    return StringFmt("r%d", *((int *) reg));
}

void reg_delete(void *reg) { 
    free(reg); 
}

