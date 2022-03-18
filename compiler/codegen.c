#include "error.h"
#include "codegen.h"
#include "statistics.h"
#include "instructions.h"
#include "irtprinter.h"

#define DEBUG 0
#define CT_BEGIN 0x0
#define CT_END   0x1

static void gen_proc        (FILE *, structures, frame, list);
static void gen_stmt        (FILE *, structures, frame, i_stmt, bool);
static void gen_jump        (FILE *, i_stmt);
static void gen_cjump       (FILE *, i_stmt);
static void gen_move        (FILE *, structures, frame, i_stmt);
static void gen_input       (FILE *, i_stmt);
static void gen_output      (FILE *, i_stmt);
static void gen_fork        (FILE *, i_stmt);
static void gen_forkSet     (FILE *, i_stmt);
static void gen_forkSync    (FILE *, i_stmt);
static void gen_join        (FILE *, i_stmt);
static void gen_on          (FILE *, structures, frame, i_stmt);
static void gen_connect     (FILE *, frame, i_stmt);
static void gen_return      (FILE *, structures, i_stmt, bool);
static void gen_fnCall      (FILE *, structures, frame, i_expr, int);
static void gen_pCall       (FILE *, structures, frame, i_stmt);
static void gen_binop       (FILE *, structures, int, i_expr);
static void gen_temp        (FILE *, int, i_expr);
static void gen_const       (FILE *, structures, int, i_expr);
static void gen_globalLoad  (FILE *, i_expr, int);
static void gen_globalStore (FILE *, i_expr, int);
static void gen_addr        (FILE *, frame, i_expr, int);
static void gen_load        (FILE *, frame, i_expr, i_expr);
static void gen_store       (FILE *, frame, i_expr, i_expr);
static void gen_data        (FILE *, structures, list);
static void gen_consts      (FILE *, list);
static void gen_jumpTable   (FILE *, structures);
//static void gen_constRefs   (FILE *, list);

static string procSizesLblStr();
static string procBottomLblStr(string);
static string procJumpLblStr();

// Main method to generate the program
void gen_program(FILE *asmOut, FILE *jumpTabOut, FILE *cpOut, structures s) {
    
    // Put main proc at top
    list_insertFirst(s->ir->procs, 
        list_remove(s->ir->procs, LBL_MAIN, &isNamedProc)); 

    // Label processes (for branch link backwards/forwards)
    int i = 0;
    iterator it = it_begin(s->ir->procs);
    while(it_hasNext(it)) {
        ir_proc proc = it_next(it);
        proc->pos = i++; 
    }
    it_free(&it);

    emit(asmOut, ".extern %s", LBL_MIGRATE);
    emit(asmOut, ".extern %s", LBL_INIT_THREAD);
    emit(asmOut, ".extern %s", LBL_CHAN_ARRAY);
    emit(asmOut, "\n.text\n");

    emit(asmOut, ".globl %s, \"f{0}(0)\"", LBL_MAIN);

    // Transform, allocate regs and emit instructions
    it = it_begin(s->ir->procs);
    while(it_hasNext(it)) {
        ir_proc proc = it_next(it);
    
        // Generate instructions
        if(DEBUG) printf("Generating proc %s: %d\n", frm_name(proc->frm), proc->pos);
        gen_proc(asmOut, s, proc->frm, proc->stmts.ir);
    }
    it_free(&it);
   
    // Genrate references for constants
    //gen_constRefs(asmOut, s->ir->consts);
    
    // Generate global data section
    gen_data(asmOut, s, s->ir->data);
   
    // Generate jump table in a seperate file
    gen_jumpTable(jumpTabOut, s);
    
    // Generate constants in a seperate file
    gen_consts(cpOut, s->ir->consts);
}

// Generate a sequence of assembly instructions
void gen_proc(FILE *out, structures s, frame frm, list stmts) {

    string name = frm_name(frm);

    // Assign an ordering to the statements and labels
    int i = 0;
    iterator it = it_begin(stmts);
    while(it_hasNext(it)) {
        i_stmt stmt = it_next(it);
        stmt->pos = i;
        if(stmt->type == t_LABEL)
            lbl_setPos(stmt->u.LABEL, i);
        i++;
    }
    it_free(&it);

    // Begin emission of procedure instructions
    emit_sec(out, name);
    emit(out, ".globl %s", name);
    emit(out, ".cc_top %s.function,%s\n", name, name);
    emit(out, ".align 4"); // Word-align procedures so they can be read from memory
    fprintf(out, "%s:\n", name);

    frm_genPrologue(frm, out);

    it = it_begin(stmts);
    while(it_hasNext(it)) {
        i_stmt stmt = it_next(it);
        if(DEBUG) { printf("%d: ", stmt->pos); p_stmt(stdout, 0, stmt); }
        gen_stmt(out, s, frm, stmt, !it_hasNext(it));
    }
    it_free(&it);

    frm_genEpilogue(frm, out);
    fprintf(out, "\n%s:\n", procBottomLblStr(name));
    emit(out, ".cc_bottom %s.function\n", name);
    emit(out, "");

    stat_numInstructions += list_size(stmts);
}

//========================================================================
// Statements
//========================================================================

// Statement
static void gen_stmt(FILE *out, structures s, frame f, i_stmt stmt, bool last) {
    //printf("gen stmt %d\n", stmt->pos);
    switch (stmt->type) {
    case t_LABEL:  
        fprintf(out, "%s:\n", lbl_name(stmt->u.LABEL)); break;
    case t_JUMP:     gen_jump(out, stmt);               break;
    case t_CJUMP:    gen_cjump(out, stmt);              break;
    case t_MOVE:     gen_move(out, s, f, stmt);         break;
    case t_INPUT:    gen_input(out, stmt);              break;
    case t_OUTPUT:   gen_output(out, stmt);             break;
    case t_FORK:     gen_fork(out, stmt);               break;
    case t_FORKSET:  gen_forkSet(out, stmt);            break;
    case t_FORKSYNC: gen_forkSync(out, stmt);           break;
    case t_JOIN:     gen_join(out, stmt);               break;
    case t_ON:       gen_on(out, s, f, stmt);           break;
    case t_CONNECT:  gen_connect(out, f, stmt);         break;
    case t_RETURN:   gen_return(out, s, stmt, last);    break;
    case t_PCALL:    gen_pCall(out, s, f, stmt);        break;
    case t_END:                                         break;
    default: assert(0 && "Invalid stat type");
    }
}

// Branch
static void gen_jump(FILE *out, i_stmt stmt) {
    label l = stmt->u.JUMP->u.NAME;
    
    if(stmt->pos < lbl_pos(l))
        emit_l(out, i_BU, lbl_name(l));
    else
        emit_l(out, i_BU, lbl_name(l));
}

// Conditional branch
static void gen_cjump(FILE *out, i_stmt stmt) {
    temp t = stmt->u.CJUMP.expr->u.TEMP;
    label trueBranch = (label) stmt->u.CJUMP.then->u.NAME;

    if(stmt->pos < lbl_pos(trueBranch))
        emit_1rl(out, i_BT, tmp_reg(t), lbl_name(trueBranch));
    else
        emit_1rl(out, i_BT, tmp_reg(t), lbl_name(trueBranch));
}

// Move
static void gen_move(FILE *out, structures s, frame f, i_stmt stmt) {

    i_expr dst = stmt->u.MOVE.dst;
    i_expr src = stmt->u.MOVE.src;

    // MEM(x) <-- (TEMP | CONST)
    switch(dst->type) {
    case t_MEM:
        if(src->type == t_TEMP || src->type == t_CONST)
            gen_store(out, f, src, dst);
        else assert(0 && 
                "Invalid move src i_expr type for source t_MEM");
        break;

    // TEMP <-- (TEMP | CONST | BINOP | FCALL | MEM(x) | NAME)
    case t_TEMP: {
        int dstReg = tmp_reg(dst->u.TEMP);
        switch(src->type) {
        case t_TEMP:   gen_temp    (out, dstReg, src);       break;
        case t_CONST:  gen_const   (out, s, dstReg, src);    break;
        case t_BINOP:  gen_binop   (out, s, dstReg, src);    break;
        case t_MEM:    gen_load    (out, f, dst, src);       break;
        case t_FCALL:  gen_fnCall  (out, s, f, src, dstReg); break;
        default: assert(0 &&
                "Invalid move src i_expr type for source t_TEMP");
        }
        break;
    }

    default: assert(0 && "invalid move dst i_expr type");
    }
}

// Input operation
static void gen_input(FILE *o, i_stmt stmt) {
    
    i_expr dst = stmt->u.IO.dst;
    i_expr src = stmt->u.IO.src;

    assert(dst->type == t_TEMP && "output src not TEMP");
    
    switch(src->type) {
    case t_TEMP:
        emit_2r(o, i_IN, tmp_reg(src->u.TEMP), tmp_reg(dst->u.TEMP));
        break;
    case t_SYS: {
        int offreg = tmp_reg(src->u.SYS.value->u.TEMP);
        emit_1rl(o, i_LDAWDP, 11, LBL_CHAN_ARRAY);
        emit_3r (o, i_LDW, 11, 11, offreg);
        emit_1ru(o, i_CHKCT, 11, CT_BEGIN); 
        emit_1ru(o, i_OUTCT, 11, CT_BEGIN); 
        emit_2r (o, i_OUT, 11, tmp_reg(dst->u.TEMP));
        emit_1ru(o, i_CHKCT, 11, CT_END); 
        emit_1ru(o, i_OUTCT, 11, CT_END); 
        break;
    }
    default: assert("output dst not TEMP or SYS");
    }
}

// Output operation
static void gen_output(FILE *o, i_stmt stmt) {
    
    i_expr dst = stmt->u.IO.dst;
    i_expr src = stmt->u.IO.src;

    assert(src->type == t_TEMP && "output src not TEMP");
   
    switch(dst->type) {
    case t_TEMP:
        emit_2r(o, i_OUT, tmp_reg(dst->u.TEMP), tmp_reg(src->u.TEMP));
        break;
    case t_SYS: {
        int offreg = tmp_reg(dst->u.SYS.value->u.TEMP);
        emit_1rl(o, i_LDAWDP, 11, LBL_CHAN_ARRAY);
        emit_3r (o, i_LDW, 11, 11, offreg);
        emit_1ru(o, i_OUTCT, 11, CT_BEGIN); 
        emit_1ru(o, i_CHKCT, 11, CT_BEGIN); 
        emit_2r (o, i_OUT, 11, tmp_reg(src->u.TEMP));
        emit_1ru(o, i_OUTCT, 11, CT_END); 
        emit_1ru(o, i_CHKCT, 11, CT_END); 
        break;
    }
    default: assert("output dst not TEMP or SYS");
    }
}

// Fork a set of synchronous threads
static void gen_fork(FILE *out, i_stmt stmt) {
    emit(out, "%s Begin fork", ASM_COMMENT);

    assert(stmt->u.FORK.t1->type == t_TEMP && "fork t1 not TEMP");
    assert(stmt->u.FORK.t2->type == t_TEMP && "fork t2 not TEMP");
    assert(stmt->u.FORK.t3->type == t_TEMP && "fork t3 not TEMP");

    int syncReg = tmp_reg(stmt->u.FORK.t1->u.TEMP);
    int spaceReg = tmp_reg(stmt->u.FORK.t3->u.TEMP); 
    
    // Get a synchroniser
    emit_1ru(out, i_LDC, spaceReg, THREAD_STACK_SPACE);
    emit_1ru(out, i_GETR, syncReg, RES_TYPE_SYNC);
}

// Setup a synchronous thread
static void gen_forkSet(FILE *out, i_stmt stmt) {
        
    int sync = tmp_reg(stmt->u.FORKSET.sync->u.TEMP);
    int thread = tmp_reg(stmt->u.FORKSET.thread->u.TEMP); 
    int space = tmp_reg(stmt->u.FORKSET.space->u.TEMP); 
    
    // Get a synchronised thread
    emit_2r (out, i_GETST, sync, thread);

    // Setup pc = &initThread (from jump table)
    emit_1ru(out, i_LDWCP, REG_GDEST, JUMPI_INIT_THREAD);
    emit_2r (out, i_TINITPC, REG_GDEST, thread);

    // Set lr = &instruction label
    emit_l  (out, i_LDAP,    lbl_name(stmt->u.FORKSET.l));
    emit_2r (out, i_TINITLR, REG_GDEST, thread);

    // Move sp away: sp -= 0x1000 and save it
    emit_1rl(out, i_LDWDP,   REG_GDEST, "sp");
    emit_3r (out, i_SUB,     REG_GDEST, REG_GDEST, space);
    emit_1rl(out, i_STWDP,   REG_GDEST, "sp");
    emit_2r (out, i_TINITSP, REG_GDEST, thread);
    
    // Setup dp, cp
    emit_1ru(out, i_LDAWDP,  REG_GDEST, 0);
    emit_2r (out, i_TINITDP, REG_GDEST, thread);
    emit_1ru(out, i_LDAWCP,  REG_GDEST, 0);
    emit_2r (out, i_TINITCP, REG_GDEST, thread);

    // Copy register values
    emit_3r (out, i_TSETR, thread, 0,  0);
    emit_3r (out, i_TSETR, thread, 1,  1);
    emit_3r (out, i_TSETR, thread, 2,  2);
    emit_3r (out, i_TSETR, thread, 3,  3);
    emit_3r (out, i_TSETR, thread, 4,  4);
    emit_3r (out, i_TSETR, thread, 5,  5);
    emit_3r (out, i_TSETR, thread, 6,  6);
    emit_3r (out, i_TSETR, thread, 7,  7);
    emit_3r (out, i_TSETR, thread, 8,  8);
    emit_3r (out, i_TSETR, thread, 9,  9);
    emit_3r (out, i_TSETR, thread, 10, 10);
    emit_3r (out, i_TSETR, thread, 11, 11);
}

// Synchronise (run) threads
static void gen_forkSync(FILE *out, i_stmt stmt) {

    int syncReg = tmp_reg(stmt->u.FORKSYNC.sync->u.TEMP);
    emit_1r (out, i_MSYNC, syncReg);
    emit(out, "%s end fork", ASM_COMMENT);
}

// Join a set of synchronised threads
static void gen_join(FILE *out, i_stmt stmt) {
    if(stmt->u.JOIN.master) {
        emit_1r(out, i_MJOIN, tmp_reg(stmt->u.JOIN.t1->u.TEMP));
        emit_l (out, i_BU,    lbl_name(stmt->u.JOIN.exit));
    }
    else
        emit_0r(out, i_SSYNC);
}

// On statement
// ============
static void gen_on(FILE *o, structures s, frame f, i_stmt stmt) {
    
    emit(o, "/* begin on */");
  
    i_expr pCall = stmt->u.ON.pCall;
    string procName = lbl_name(pCall->u.FCALL.func->u.NAME);
    i_expr dest = stmt->u.ON.dest;
    list args = stmt->u.ON.pCall->u.FCALL.args;
    ir_proc proc = list_getFirst(s->ir->procs, procName, &isNamedProc);
    int spOff = frm_outArgOff(f);
    int closureOffset = spOff;
    assert((spOff == 1 || spOff == 0) && "outArg offset should be 0 or 1");

    // Calculate the size of the closure and extend the frame to accommodate it
    int numProcs = 1 + list_size(ir_childCalls(proc));
    // |header| + 2*#args + #procs + |preserved|
    int closureSize = 2 + 2*list_size(args) + numProcs + 3;
    int extend = closureSize - frm_numOutArgs(f);
    
    // If we need to extend, save the original sp value
    if(extend) {
        emit_1ru(o, i_LDAWSP, 11, 0);
        emit_1ru(o, i_STWSP, 11, frm_outArgOff(f));
        emit_u(o, i_EXTSP, extend);
    }

    // sp[c] := number of arguments
    emit_1ru(o, i_LDC, 11, list_size(args));
    emit_1ru(o, i_STWSP, 11, spOff++);

    // sp[c+1] := number of procedures
    emit_1ru(o, i_LDC, 11, numProcs);
    emit_1ru(o, i_STWSP, 11, spOff++);

    // sp[c+2] ... sp[c+2+|args|]: arguments
    signature sig = sigTab_lookup(s->sig, frm_name(proc->frm));
    int i=0;
    iterator it = it_begin(args);
    while(it_hasNext(it)) {
        
        i_expr arg = it_next(it);
        assert(arg->type == t_TEMP && "arg not TEMP");
        t_formal type = sig_getFmlType(sig, i++);
        
        // Array: (size, &array). Next argument must be the array length
        if(type == t_formal_intArray || type == t_formal_chanArray) {
            i_expr nextArg = it_peekNext(it);
            assert(nextArg!=NULL && "array formal must be followed by length");
            emit_1ru(o, i_STWSP, tmp_reg(nextArg->u.TEMP), spOff++);
            emit_1ru(o, i_STWSP, tmp_reg(arg->u.TEMP), spOff++);
        }
        // Value: (1, value)
        else {
            emit_1ru(o, i_LDC, 11, 1);
            emit_1ru(o, i_STWSP, 11, spOff++);
            emit_1ru(o, i_STWSP, tmp_reg(arg->u.TEMP), spOff++);
        }
    }
    it_free(&it);

    // sp[c+4] := jump index of the procedure
    emit_1ru(o, i_LDC,   11, proc->pos+JUMP_INDEX_OFFSET);
    emit_1ru(o, i_STWSP, 11, spOff++);

    // sp[c+2+|args|+1] ... sp[c+2+|args|+1+|children|]
    it = it_begin(ir_childCalls(proc));
    while(it_hasNext(it)) {
        ir_proc child = it_next(it);
        //string childName = frm_name(child->frm);

        emit_1ru(o, i_LDC,   11, child->pos+JUMP_INDEX_OFFSET);
        emit_1ru(o, i_STWSP, 11, spOff++);
    }
    it_free(&it);

    // Preserve r0, r1, r2
    emit_1ru(o, i_STWSP, 0, spOff++);
    emit_1ru(o, i_STWSP, 1, spOff++);
    emit_1ru(o, i_STWSP, 2, spOff++);
    
    assert((spOff - closureOffset == closureSize) && "invlalid closure size");

    // r0 (arg1) := destination
    emit_2r(o, i_MOVE, 0, tmp_reg(dest->u.SYS.value->u.TEMP));

    // r1 (arg2) := address of closure
    emit_1ru(o, i_LDAWSP, 1, closureOffset);

    // r2 (arg3) := closure array length (hidden array length argument)
    emit_1ru(o, i_LDC, 2, closureSize);

    // Call the migration routine
    emit(o, "bla cp[%d]", JUMPI_MIGRATE);
       
    // Restore saved registers
    emit_1ru(o, i_LDWSP, 0, spOff-3);
    emit_1ru(o, i_LDWSP, 1, spOff-2);
    emit_1ru(o, i_LDWSP, 2, spOff-1);
    
    // Contract stack
    if(extend) {
        emit_1ru(o, i_LDWSP, 11, extend+frm_outArgOff(f));
        emit_1r (o, i_SETSP, 11);
    }

    emit(o, "/* end on */");
}

// Generate a connect statement
static void gen_connect(FILE *o, frame f, i_stmt stmt) {
    
    i_expr to = stmt->u.CONNECT.to;
    i_expr c1 = stmt->u.CONNECT.c1;
    i_expr c2 = stmt->u.CONNECT.c2;

    // Check resources are SYS
    assert(to->type == t_SYS && "CONNECT to not SYS"); 
    assert(c1->type == t_SYS && "CONNECT c1 not SYS"); 
    assert(c2->type == t_SYS && "CONNECT c2 not SYS");
   
    // Check the values of each are TEMPS
    assert(to->u.SYS.value->type == t_TEMP && "CONNECT to.value not TEMP");
    assert(c1->u.SYS.value->type == t_TEMP && "CONNECT c1.value not TEMP");
    assert(c2->u.SYS.value->type == t_TEMP && "CONNECT c2.value not TEMP");

    emit(o, "/* begin connect */");

    // Preserve r0, r1, r2
    int offset = frm_preservedOff(f);
    emit_1ru(o, i_STWSP, 0, offset);
    emit_1ru(o, i_STWSP, 1, offset+1);
    emit_1ru(o, i_STWSP, 2, offset+2);
  
    // Move the temps into the parameter regs
    gen_temp(o, 0, to->u.SYS.value);
    gen_temp(o, 1, c1->u.SYS.value);
    gen_temp(o, 2, c2->u.SYS.value);

    // Call connect
    emit(o, "bla cp[%d]", JUMPI_CONNECT);
    
    // Restore r0, r1, r2
    emit_1ru(o, i_LDWSP, 0, offset);
    emit_1ru(o, i_LDWSP, 1, offset+1);
    emit_1ru(o, i_LDWSP, 2, offset+2);
    
    emit(o, "/* end connect */");
}

// Generate a function return statement
static void gen_return(FILE *out, structures s, i_stmt stmt, bool last) {
    i_expr expr = stmt->u.RETURN.expr;
  
    // Move the return value to r0
    switch (expr->type) {
    case t_BINOP: gen_binop(out, s, RETURN_REG, expr); break;
    case t_CONST: gen_const(out, s, RETURN_REG, expr); break;
    case t_TEMP:  gen_temp(out, RETURN_REG, expr);     break;
    default: assert(0 && "Invalid return i_expr type");
    }
    
    // Jump to the epilogue (will always be at end)
    if(!last)
        emit_l(out, i_BU, lbl_name(stmt->u.RETURN.end->u.NAME));
}

// Generate a procedure call
static void gen_pCall(FILE *out, structures s, frame f, i_stmt stmt) {
    string name = lbl_name(stmt->u.PCALL.proc->u.NAME);
    ir_proc p = list_getFirst(s->ir->procs, name, &isNamedProc);
    frm_genCall(f, stmt->u.PCALL.proc, stmt->u.PCALL.args, -1, p->pos, out);
}

// Generate a function call; work out if it's a forward of backwards reference
static void gen_fnCall(FILE *out, structures s, frame f, i_expr expr, int dstReg) {
    string name = lbl_name(expr->u.FCALL.func->u.NAME);
    ir_proc p = list_getFirst(s->ir->procs, name, &isNamedProc);
    frm_genCall(f, expr->u.FCALL.func, expr->u.FCALL.args, dstReg, p->pos, out);
}

//========================================================================
// Expressions
//========================================================================

// Generate a binary operation
static void gen_binop(FILE *out, structures s, int dstReg, i_expr binop) {
    
    int regA, regB, imm;
    bool isImm = false;

    t_binop opType = binop->u.BINOP.op;
    i_expr left = binop->u.BINOP.left;
    i_expr right = binop->u.BINOP.right;

    // BINOP CONST CONST should have been evaluated
    assert(!(left->type == t_CONST && right->type == t_CONST) 
            && "Invalid BINOP with CONST operands");

    // Operand A: (TEMP | CONST | MEM)
    switch(left->type) {
    case t_TEMP:  regA = tmp_reg(left->u.TEMP); break;
    case t_CONST:
        regA = dstReg;
        gen_const(out, s, regA, left);
        break;
    default: assert(0 && "invalid left BINOP expression");
    }

    // Operand B: (TEMP | CONST | MEM)
    switch(right->type) {
    case t_TEMP: regB = tmp_reg(right->u.TEMP); break;
    case t_CONST:
        imm = right->u.CONST;
        isImm = gen_inImmRangeS(imm)
            && (opType == i_plus
             || opType == i_minus
             || opType == i_eq
             || opType == i_lshift
             || opType == i_rshift);
        if(!isImm) {
            regB = dstReg;
            gen_const(out, s, regB, right);
        }
        break;
    default: assert(0 && "invalid right BINOP expression");
    }

    // Emit instruction
    t_inst op;
    if(isImm) {
        switch(opType) {
        case i_plus:   op = i_ADDI; break;
        case i_minus:  op = i_SUBI; break;
        case i_eq:     op = i_EQI;  break;
        case i_lshift: op = i_SHLI; break;
        case i_rshift: op = i_SHRI; break;
        default: assert(0 && "unrecognised imm BINOP");
        }
        emit_2ru(out, op, dstReg, regA, imm);
    }
    else {
    
        // Emit some extra and adjusted BINOPs ad we're not doing conditionals
        // properly
        switch(opType) {
        case i_ne: 
            emit_3r(out,  i_EQ, dstReg, regA, regB);
            emit_2ru(out, i_EQI, dstReg, dstReg, 0);
            break;
        case i_le: 
            emit_3r(out,  i_LSS, dstReg, regB, regA);
            emit_2ru(out, i_EQI, dstReg, dstReg, 0);
            break;
        case i_gr:
            emit_3r(out,  i_LSS, dstReg, regB, regA);
            break;
        case i_ge:
            emit_3r(out,  i_LSS, dstReg, regA, regB);
            emit_2ru(out, i_EQI, dstReg, dstReg, 0);
            break;
    
        // Regular BINOPs
        default:
            switch(opType) {
            case i_plus:   op = i_ADD;  break; 
            case i_minus:  op = i_SUB;  break; 
            case i_mult:   op = i_MUL;  break;
            case i_div:    op = i_DIVS; break;
            case i_rem:    op = i_REMS; break;
            case i_or:     op = i_OR;   break; 
            case i_and:    op = i_AND;  break; 
            case i_xor:    op = i_XOR;  break;
            case i_lshift: op = i_SHL;  break; 
            case i_rshift: op = i_SHR;  break;
            case i_eq:     op = i_EQ;   break; 
            case i_ls:     op = i_LSS;  break;
            default: assert(0 && "unrecognised BINOP");
            }
            emit_3r(out, op, dstReg, regA, regB);
        }
    }
}

//========================================================================
// Loads and stores
//========================================================================

// Generate a temp (i.e. register value) in another register
static void gen_temp(FILE *out, int dstReg, i_expr temp) {
    assert(temp->type == t_TEMP && "i_expr not of type temp");
    if(tmp_reg(temp->u.TEMP) != dstReg)
        emit_2r(out, i_MOVE, dstReg, tmp_reg(temp->u.TEMP));
}

// Generate a constant value
static void gen_const(FILE *out, structures s, int reg, i_expr constant) {

    unsigned int constVal = constant->u.CONST;
    // 16-bit imm
    if(constVal <= 0xFFFF) {
        //printf("loading %x\n", constVal);
        emit_1ru(out, i_LDC, reg, constVal);
    }
    // Add to constant pool and load from there
    else {
        label l = ir_NewConst(s->ir, s->lbl, constVal);
        //int offset = ir_cpOff(s->ir, lbl_name(l));
        emit_1rl(out, i_LDWCP, reg, lbl_name(l));
    }
}

// Return the offset to an area of the stack
static int getStackOffset(frame f, t_memBase base) {
    switch(base) {
    case t_mem_spo: return frm_outArgOff(f);   
    case t_mem_spl: return frm_localOff(f);    
    case t_mem_spp: return frm_preservedOff(f);
    case t_mem_spi: return frm_inArgOff(f);    
    default: 
        assert(0 && "invalid MEM base");
        return -1;
    }
}

// Generate a load relative to dp or cp
// dstReg := mem[dp/cp + base + offset]
static void gen_globalLoad(FILE *out, i_expr mem, int dstReg) {
    
    assert(mem->u.MEM.base->type == t_NAME 
            && "load MEM expr type not t_NAME");
    
    label l = mem->u.MEM.base->u.NAME;
    
    switch(mem->u.MEM.offset->type) {
    
    // If offset is a constant
    case t_CONST:
        switch(mem->u.MEM.type) {
        case t_mem_dp: emit_1rl(out, i_LDWDP, dstReg, lbl_name(l)); break;
        case t_mem_cp: emit_1rl(out, i_LDWCP, dstReg, lbl_name(l)); break;
        default: assert(0 && "invalid memBase for global load");
        }
        break;
   
    // If offset is a variable
    case t_TEMP:
        switch(mem->u.MEM.type) {
        case t_mem_dp:
            emit_1rl(out, i_LDAWDP, dstReg, lbl_name(l));
            emit_3r(out, i_LDW, dstReg, dstReg,
                    tmp_reg(mem->u.MEM.offset->u.TEMP));
            break;
        case t_mem_cp: 
            emit_1rl(out, i_LDAWCP, dstReg, lbl_name(l));
            emit_3r(out, i_LDW, dstReg, dstReg,
                    tmp_reg(mem->u.MEM.offset->u.TEMP));
            break;
        default: assert(0 && "invalid memBase for global load");
        }
        break;
    
    default: assert(0 && "invalid MEM offset for globalLoad");
    }
}

// Generate a load relative to dp or cp
// mem[dp/cp + base + offset] := srcReg
static void gen_globalStore(FILE *out, i_expr mem, int srcReg) {

    assert(mem->u.MEM.base->type == t_NAME 
            && "store MEM expr type not t_NAME");

    int offset; 
    label l = mem->u.MEM.base->u.NAME;
    
    switch(mem->u.MEM.offset->type) {

    // If offset is a constant
    case t_CONST:
        switch(mem->u.MEM.type) {
        case t_mem_dp:
            offset = mem->u.MEM.offset->u.CONST;
            assert(gen_inImmRangeL(offset) && "dp offset > 16 bits");
            emit_1rl(out, i_LDAWDP, REG_GDEST, lbl_name(l));
            emit_2ru(out, i_STWI, srcReg, REG_GDEST, offset);
            break;
        /*case t_mem_cp:
            offset = ir_cpOff(s->ir, lbl_name(l));
            offset += mem->u.MEM.offset->u.CONST;
            assert(gen_inImmRangeL(offset) && "cp offset > 16 bits");
            emit_1ru(out, i_STWCP, srcReg, offset);
            break;*/
        default: assert(0 && "invalid memBase for global store");
        }
        break;

    // If offset is a variable
    case t_TEMP:
        switch(mem->u.MEM.type) {
        case t_mem_dp:
            emit_1rl(out, i_LDAWDP, REG_GDEST, lbl_name(l));
            emit_3r(out, i_STW, srcReg, REG_GDEST,
                    tmp_reg(mem->u.MEM.offset->u.TEMP));
            break;
        case t_mem_cp: 
            emit_1rl(out, i_LDAWCP, REG_GDEST, lbl_name(l));
            emit_3r(out, i_STW, srcReg, REG_GDEST,
                    tmp_reg(mem->u.MEM.offset->u.TEMP));
            break;
        default: assert(0 && "invalid memBase for global load");
        }
        break;

    default: assert(0 && "invalid MEM offset for globalStore");
    }
}

// Generate an address relative to dp or cp
// dstReg := &mem[dp/cp + base + offset]
static void gen_addr(FILE *out, frame f, i_expr mem, int dstReg) {
   
    int off;
    i_expr base = mem->u.MEM.base;
    i_expr offset = mem->u.MEM.offset;
    
    assert(offset->type == t_CONST && "offset not CONST for address load");

    switch(offset->type) {

    // If offset is a constant
    case t_CONST:
        switch(mem->u.MEM.type) {
        case t_mem_spa:
            off = offset->u.CONST;
            off = frm_arrayOff(f);
            emit_1ru(out, i_LDAWSP, dstReg, off);
            break;
        case t_mem_dpa:
            assert(base->type == t_NAME 
                    && "address MEM expr type not t_NAME");
            emit_1rl(out, i_LDAWDP, dstReg, lbl_name(base->u.NAME));
            break;
        case t_mem_cpa:
            assert(base->type == t_NAME 
                    && "address MEM expr type not t_NAME");
            emit_l(out, i_LDAWCP, lbl_name(base->u.NAME));
            break;
        default: assert(0 && "invalid base for address load");
        }
        break;

    // If offset is a variable - this won't happen as only arrays are passed by
    // reference
    case t_TEMP:
        switch(mem->u.MEM.type) {
        case t_mem_spa:
            break;
        case t_mem_dpa:
            break;
        case t_mem_cpa:
            break;
        default: assert(0 && "invalid base for address load");
        }
        break;
    
    default: assert(0 && "invalid MEM offset for address load");
    }

}

// Generate a load from mem into dst
static void gen_load(FILE *out, frame f, i_expr dst, i_expr mem) {

    assert(dst->type == t_TEMP && "destination i_expr not of type temp");
    
    temp t = dst->u.TEMP;
    i_expr offset = mem->u.MEM.offset;
    i_expr base = mem->u.MEM.base;
    int baseReg, offReg;
    int dstReg = tmp_reg(dst->u.TEMP);

    switch(mem->u.MEM.type) {
    
    // Arbitrary load
    case t_mem_abs:
        assert(base->type == t_TEMP && offset->type == t_TEMP
                && "base and offsets not TEMPS for MEM absl ref");
        baseReg = tmp_reg(base->u.TEMP);
        offReg = tmp_reg(offset->u.TEMP);
        emit_3r(out, i_LDW, dstReg, baseReg, offReg);
        break;

    // Relative to sp
    case t_mem_spo: case t_mem_spl:
    case t_mem_spp: case t_mem_spi: {
        int off = getStackOffset(f, mem->u.MEM.type);
        switch(offset->type) {
        case t_CONST:
            off += offset->u.CONST;
            emit_1ru(out, i_LDWSP, tmp_reg(t), off);
            break;
        case t_TEMP:
            assert(mem->u.MEM.base->type == t_TEMP && "MEM base not TEMP");
            baseReg = tmp_reg(mem->u.MEM.base->u.TEMP);
            offReg = tmp_reg(offset->u.TEMP);
            emit_2ru(out, i_ADDI, offReg, offReg, off);
            emit_3r(out, i_LDW, dstReg, baseReg, offReg);
            break;
        default: assert(0 && "MEM expr not CONST or TEMP");
        }

        break;
    }

    // Element in data or constant pool
    case t_mem_dp: case t_mem_cp:
        gen_globalLoad(out, mem, dstReg);
        break;

    // Address of an element in data or constant pools
    case t_mem_spa: case t_mem_dpa: case t_mem_cpa:
        gen_addr(out, f, mem, dstReg);
        break;

    default: assert(0 && "invalid MEM load base");
    }
}

// Generate a store to mem from base sp of dp, from a source TEMP or CONST. If
// constant value < 11, can insert directly into load as immediate
static void gen_store(FILE *out, frame f, i_expr src, i_expr mem) {
    
    assert(src->type == t_TEMP && "source i_expr type not t_TEMP");

    i_expr offset = mem->u.MEM.offset;
    i_expr base = mem->u.MEM.base;
    int baseReg, offReg;
    int srcReg = tmp_reg(src->u.TEMP);

    // Local in stack
    switch(mem->u.MEM.type) {
    case t_mem_spo: case t_mem_spl:
    case t_mem_spp: case t_mem_spi: {
        int off = getStackOffset(f, mem->u.MEM.type);
        switch(offset->type) {
        case t_CONST:
            emit_1ru(out, i_STWSP, tmp_reg(src->u.TEMP), 
                    off + offset->u.CONST);
            break;
        case t_TEMP:
            assert(mem->u.MEM.base->type == t_TEMP && "MEM base not TEMP");
            baseReg = tmp_reg(mem->u.MEM.base->u.TEMP);
            offReg = tmp_reg(offset->u.TEMP);
            emit_2ru(out, i_ADDI, offReg, offReg, off);
            emit_3r(out, i_STW, srcReg, baseReg, offReg);
            break;
        default: assert(0 && "MEM expr not CONST or TEMP");
        }
        break;
    }

    // Arbitrary reference
    case t_mem_abs:
        assert(base->type == t_TEMP && offset->type == t_TEMP
                && "base and offsets not TEMPS for MEM absl ref");
        baseReg = tmp_reg(base->u.TEMP);
        offReg = tmp_reg(offset->u.TEMP);
        emit_3r(out, i_STW, srcReg, baseReg, offReg);
        break;

    // Global in data or string in constant pool
    case t_mem_dp: case t_mem_cp:
        gen_globalStore(out, mem, srcReg);
        break;

    default: assert(0 && "invalid store MEM base");
    }
}


//========================================================================
// Constant and data sections
//========================================================================

// Generate constant values
static void gen_consts(FILE *out, list consts) {

    emit_sec(out, "Constants");
    emit(out, ".section .cp.rodata, \"ac\", @progbits");
    emit(out, ".align 4");
    
    // And any constant values
    if(!list_empty(consts)) {

        // Constants
        iterator it = it_begin(consts);
        while(it_hasNext(it)) {
            ir_data c = it_next(it);
            switch(c->type) {

            // Constant value
            case t_ir_const:
                fprintf(out, "\n\t.globl %s\n", lbl_name(c->l));
                fprintf(out, "%s:\n", lbl_name(c->l));
                emit(out, ".word %d", c->u.value);
                break;
            
            // Output string bytes as ints, with the first byte as the length
            case t_ir_str:
                //fprintf(out, "%s: %s %s\n", lbl_name(c->l), ASM_COMMENT, c->u.strVal);
                /*fprintf(out, "%s:\n", lbl_name(c->l));
                emit(out, ".byte %d", strlen(c->u.strVal));
                emit(out, ".ascii \"%s\"", c->u.strVal);*/
                break;

            default: assert(0 && "invalid constant element");
            }
        }
        it_free(&it);
    }
}

// Generate jump table
static void gen_jumpTable(FILE *out, structures s) {
    
    emit_sec(out, "Jump table");

    emit(out, ".section .cp.rodata, \"ac\", @progbits");
    emit(out, ".align 4\n");
    
    // Add in the migrate and threadInit entries
    emit(out, ".globl %s, \"a(:ui)\"", procJumpLblStr());
    emit(out, ".set %s.globound, %d", procJumpLblStr(),
            BYTES_PER_WORD*JUMP_TAB_SIZE);
    fprintf(out, "%s:\n", procJumpLblStr());
    emit(out, ".word %s", LBL_MIGRATE);
    emit(out, ".word %s", LBL_INIT_THREAD);
    emit(out, ".word %s", LBL_CONNECT);

    // Jump table
    iterator it = it_begin(s->ir->procs);
    while(it_hasNext(it)) {
        ir_proc proc = it_next(it);
        string name = frm_name(proc->frm);
        emit(out, ".word %s", name);
    }
    it_free(&it);

    // Pad any unused space
    int remaining = JUMP_TAB_SIZE - 
        (JUMP_INDEX_OFFSET+list_size(s->ir->procs));
    emit(out, ".space %d", BYTES_PER_WORD * remaining);
}

// Generate constant values
/*static void gen_constRefs(FILE *out, list consts) {

    emit_sec(out, "Constant references");
    // And any constant values
    if(!list_empty(consts)) {

        // Constants
        iterator it = it_begin(consts);
        while(it_hasNext(it)) {
            ir_data c = it_next(it);
            switch(c->type) {

            // Constant value
            case t_ir_const:
                emit(out, ".extern %s", lbl_name(c->l));
                break;
            
            // Output string bytes as ints, with the first byte as the length
            case t_ir_str:
                //fprintf(out, "%s: %s %s\n", lbl_name(c->l), ASM_COMMENT, c->u.strVal);
                //fprintf(out, "%s:\n", lbl_name(c->l));
                //emit(out, ".byte %d", strlen(c->u.strVal));
                //emit(out, ".ascii \"%s\"", c->u.strVal);
                break;

            default: assert(0 && "invalid constant element");
            }
        }
        it_free(&it);
    }

    emit(out, "");
}*/

// Generate global data items
static void gen_data(FILE *out, structures s, list globals) {
    emit_sec(out, "Data");
    emit(out, ".section .dp.data, \"awd\", @progbits");
    emit(out, ".align 4\n");

    // Procedure infos
    emit(out, ".globl %s", procSizesLblStr());
    emit(out, ".set %s.globound, %d", procSizesLblStr(),
            BYTES_PER_WORD*(2+list_size(s->ir->procs)));
    fprintf(out, "%s:\n", procSizesLblStr());
    
    // Pad migrate and initThread entries
    for(int i=0; i<JUMP_INDEX_OFFSET; i++)
        emit(out, ".word 0");

    // Fill in program procedure entries
    iterator it = it_begin(s->ir->procs);
    while(it_hasNext(it)) {
        ir_proc proc = it_next(it);
        string name = frm_name(proc->frm);
        
        // TODO: +4 is a bit of hack
        emit(out, ".word %s-%s+4", procBottomLblStr(name), name);
    }
    it_free(&it);
    
    // Globals
    if(!list_empty(globals)) {
        iterator it = it_begin(globals);
        while(it_hasNext(it)) {
            ir_data g = it_next(it);
            switch(g->type) {
            case t_ir_global:
                emit(out, "\n.globl %s", lbl_name(g->l));
                fprintf(out, "%s: %s %s\n", lbl_name(g->l), ASM_COMMENT, g->name);
                emit(out, ".space %d", g->u.size * BYTES_PER_WORD);
                if(g->u.size > 1) {
                    emit(out, ".globl %s.globound", lbl_name(g->l));
                    emit(out, ".set %s.globound,%d", lbl_name(g->l), 
                            g->u.size * BYTES_PER_WORD);
                }
                break;
            default: assert(0 && "invalid global data element");
            }
        }
        it_free(&it);
    }
}

static string procSizesLblStr() {
    return "sizeTable";
}

static string procBottomLblStr(string proc) {
    return StringFmt(".L%s_bottom", proc);
}

static string procJumpLblStr() {
    return "jumpTable";
}

bool gen_inImmRangeS(int value) {
    return value < 11;
}

bool gen_inImmRangeL(int value) {
    return value < 0xFFFF;
}

// =======================================================================
// Assembly emission
// =======================================================================

// Generate a string value
/*static void emit_string(FILE *out, ir_data c) {
    emit(out, "%s: %s %s", lbl_name(c->l), ASM_COMMENT, c->u.strVal);

    string s = c->u.strVal;
    int i;
    int len = (int) strlen(s);
    assert(strlen(s) <= 0xFF && "string len > 256");

    int round4 = (len+1)%4 != 0 ? (len+1)-(len+1)%4+4 : len+1; 
    char n[round4];
    strncpy(n+1, s, len);
    n[0] = len & 0xFF;

    for(i=len+1; i<round4; i++)
        n[i] = 0;
    
    //printf("emitting %s, len 0x%02x, newlen %d\n", c->u.strVal, (int)n[0], round4);
}*/

