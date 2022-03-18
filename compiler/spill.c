#include "spill.h"
#include "block.h"
#include "statistics.h"
#include "error.h"
#include "irtprinter.h"

static void addMemAccesses(structures, frame, list blocks);
static i_expr addMemLoadsExpr(structures, frame, iterator stmtIt, i_expr);
static i_expr addLoad(structures, frame, iterator stmtIt, i_expr expr);
static i_expr addStore(structures, frame, iterator stmtIt, i_expr, i_expr);

// Main method
void spill_rewrite(structures s, frame f, list blocks) {
    addMemAccesses(s, f, blocks);
}

// For variables spilled on stack, insert memory accesses for them
static void addMemAccesses(structures s, frame frm, list blocks) {

    // For each block
    iterator blockIt = it_begin(blocks);
    while(it_hasNext(blockIt)) {
        block b = it_next(blockIt);

        // For each statement
        iterator stmtIt = it_begin(blc_stmts(b));
        while(it_hasNext(stmtIt)) {
            i_stmt stmt = it_next(stmtIt);
            //printf("stmt %d: def(%s), use(%s)\n", stmt->pos, 
            //        set_string(stmt->def, &showTmpAcc),
            //        set_string(stmt->use, &showTmpAcc));
            //printf("%d: ", stmt->pos); p_stmt(stdout, 0, stmt);
            iterator it;

            switch(stmt->type) {
            
            case t_CJUMP:
                stmt->u.CJUMP.expr = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.CJUMP.expr);
                break;
            
            case t_MOVE:
                stmt->u.MOVE.src = addMemLoadsExpr(s, frm, stmtIt, stmt->u.MOVE.src);
                stmt->u.MOVE.dst = addStore(s, frm, stmtIt, 
                        stmt->u.MOVE.src, stmt->u.MOVE.dst);
                break; 
            
            case t_INPUT:
                stmt->u.IO.src = addMemLoadsExpr(s, frm, stmtIt, stmt->u.IO.src);
                stmt->u.IO.dst = addStore(s, frm, stmtIt, 
                        stmt->u.IO.src, stmt->u.IO.dst);
                break; 
            
            case t_OUTPUT:
                stmt->u.IO.dst = addMemLoadsExpr(s, frm, stmtIt, stmt->u.IO.dst);
                stmt->u.IO.src = addStore(s, frm, stmtIt, 
                        stmt->u.IO.dst, stmt->u.IO.src);
                break; 
            
            case t_PCALL:
                it = it_begin(stmt->u.PCALL.args);
                while(it_hasNext(it)) {
                    i_expr e = it_next(it);
                    it_replace(it, addMemLoadsExpr(s, frm, stmtIt, e));
                }
                it_free(&it);
                break;
            
            case t_ON:
                stmt->u.ON.dest = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.ON.dest);
                it = it_begin(stmt->u.ON.pCall->u.FCALL.args);
                while(it_hasNext(it)) {
                    i_expr e = it_next(it);
                    it_replace(it, addMemLoadsExpr(s, frm, stmtIt, e));
                }
                it_free(&it);
                break;

            case t_CONNECT:
                stmt->u.CONNECT.to = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.CONNECT.to);
                stmt->u.CONNECT.c1 = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.CONNECT.c1);
                stmt->u.CONNECT.c2 = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.CONNECT.c2);
                break;
            
            case t_RETURN:
                stmt->u.RETURN.expr = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.RETURN.expr);
                break;
           
            // Fork defines three temporaries
            case t_FORK:
                stmt->u.FORK.t1 = addStore(s, frm, stmtIt, 
                        stmt->u.FORK.t1, stmt->u.FORK.t1);
                stmt->u.FORK.t2 = addStore(s, frm, stmtIt, 
                        stmt->u.FORK.t2, stmt->u.FORK.t2);
                stmt->u.FORK.t3 = addStore(s, frm, stmtIt, 
                        stmt->u.FORK.t3, stmt->u.FORK.t3);
                break;

            // ForkSet uses these temporaries
            case t_FORKSET:
                stmt->u.FORKSET.sync = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.FORKSET.sync);
                stmt->u.FORKSET.thread = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.FORKSET.thread);
                stmt->u.FORKSET.space = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.FORKSET.space);
                break;

            // ForkSet uses these temporaries
            case t_FORKSYNC:
                stmt->u.FORKSYNC.sync = addMemLoadsExpr(s, frm, stmtIt, 
                        stmt->u.FORKSYNC.sync);
                break;

            // Join uses first temporary
            case t_JOIN:
                stmt->u.JOIN.t1 = addMemLoadsExpr(s, frm, stmtIt, stmt->u.JOIN.t1);
                break;
            
            case t_LABEL:
            case t_JUMP:
            case t_END:
                break;
            default: assert(0 && "Invalid stmt type");
            }
        }
        it_free(&stmtIt);
    }
    it_free(&blockIt);
}

// Add a load from memory if the expr is a TEMP (Expr should be a TEMP or
// CONST), where the access type is not a reg.
// NOTE: Don't want a load at beginning of live range as no value will exist,
// loads should only precede -live- variables
static i_expr addLoad(structures s, frame frm, iterator stmtIt, i_expr expr) {
    
    if(expr->type != t_TEMP) 
        return expr;

    temp spill = expr->u.TEMP;
    
    switch(tmp_getAccess(spill)) {
    case t_tmpAccess_frame:
    case t_tmpAccess_caller:
    case t_tmpAccess_data: {
        i_stmt stmt = it_curr(stmtIt);
        temp tmp = frm_addNewTemp(frm, t_tmp_local);
        tmp_setSpilled(tmp, tmp_name(spill));

        // Create the MOVE statement
        switch(tmp_getAccess(spill)) {
        case t_tmpAccess_frame:
            stmt = i_Move(i_Temp(tmp), i_Mem(t_mem_spl, NULL, 
                        i_Const(tmp_off(spill))));
            break;
        
        case t_tmpAccess_caller:
            stmt = i_Move(i_Temp(tmp), i_Mem(t_mem_spi, NULL, 
                        i_Const(tmp_off(spill))));
            break;
        
        case t_tmpAccess_data:
            stmt = i_Move(i_Temp(tmp), i_Mem(t_mem_dp, NULL,
                        i_Name(ir_dpLoc(s->ir, tmp_name(spill)))));
            break;
       
        default: assert(0 && "invalid tmp access type");
        }

        // Insert the load before the statement
        it_insertBefore(stmtIt, stmt);

        // Change temp to loaded one
        //expr->type = t_TEMP;
        //expr->u.TEMP = tmp;
       
        //stmt = it_curr(stmtIt); 
        //printf("inserted load for %s at stmt %d, temp access type %d\n",
        //        tmp_name(spill), stmt->pos, tmp_getAccess(spill));
        return i_Temp(tmp);
    }
    case t_tmpAccess_none:
    case t_tmpAccess_reg:
        return expr;

    case t_tmpAccess_undefined:
        assert(0 && "Undefined tmpAccess type");
    default:
        assert(0 && "Invalid tmpAccess type");
    }
}

// Add a store to memory if the src is a TEMP
static i_expr addStore(structures s, frame frm, iterator stmtIt, i_expr src, i_expr dest) {
    if(dest->type != t_TEMP)
        return dest;

    temp spill = dest->u.TEMP;
    if(tmp_getAccess(spill) == t_tmpAccess_reg)
        return dest;
       
    // If it tries to store an already spilled variable then we have run
    // out of registers
    if(src->type == t_MEM)
        err_fatal("insufficient regsiters");

    // If undefined, then it must be a global (dead statements are elminated
    if(tmp_getAccess(spill) == t_tmpAccess_undefined) {
        if(tmp_type(spill) == t_tmp_global) {
            tmp_setDataAccess(spill);
            return dest;
        }
        // If a temp has undefined local access, it is not used
        //else if(tmp_type(spill) == t_tmp_local)
        //    tmp_setAccessNone(spill);
        else assert(0 && "Invalid tmpAccess type");
    }

    // If move contains a BINOP, FNCALL or CONST, add a store to MEM after
    if(src->type == t_BINOP || src->type == t_CONST || src->type == t_FCALL) {
        temp tmp = frm_addNewTemp(frm, t_tmp_local);
        tmp_setSpilled(tmp, tmp_name(spill));
        i_stmt stmt;

        // Create the MOVE statement
        switch(tmp_getAccess(spill)) {
        
        case t_tmpAccess_frame:
            stmt = i_Move(i_Mem(t_mem_spl, NULL, 
                        i_Const(tmp_off(spill))), i_Temp(tmp));
            break;
        case t_tmpAccess_caller:
            stmt = i_Move(i_Mem(t_mem_spi, NULL, 
                        i_Const(tmp_off(spill))), i_Temp(tmp));
            break;
        case t_tmpAccess_data:
            stmt = i_Move(i_Mem(t_mem_dp, NULL, i_Name(ir_dpLoc(s->ir,
                                tmp_name(spill)))), i_Temp(tmp));
            break;

        default: assert(0 && "invalid temp access type");
        }

        // Insert the load before the statement
        it_insertAfter(stmtIt, stmt);

        // Change temp to loaded one
        //dest->type = t_TEMP;
        //dest->u.TEMP = tmp;

        return i_Temp(tmp);
    }
    // Othewrise, just change the destination
    else {
        dest->type = t_MEM;
        if(tmp_getAccess(spill) == t_tmpAccess_frame) {
            //dest->u.MEM.type = t_mem_spl;
            //dest->u.MEM.offset = i_Const(tmp_off(spill));
            return i_Mem(t_mem_spl, NULL, i_Const(tmp_off(spill)));
            //printf("inserted local store for %s at stmt %d, temp access type %d\n",
            //    tmp_name(spill), ((i_stmt)it_curr(stmtIt))->pos, tmp_getAccess(spill));
        }
        else if(tmp_getAccess(spill) == t_tmpAccess_caller) {
            //dest->u.MEM.type = t_mem_spi;
            //dest->u.MEM.offset = i_Const(tmp_off(spill));
            return i_Mem(t_mem_spi, NULL, i_Const(tmp_off(spill)));
            //printf("inserted local store for %s at stmt %d, temp access type %d\n",
            //    tmp_name(spill), ((i_stmt)it_curr(stmtIt))->pos, tmp_getAccess(spill));
        }
        else if(tmp_getAccess(spill) == t_tmpAccess_data) {
            //dest->u.MEM.type = t_mem_dp;
            //dest->u.MEM.offset = i_Name(ir_dpLoc(s->ir, tmp_name(spill)));
            return i_Mem(t_mem_dp, NULL, i_Name(ir_dpLoc(s->ir, tmp_name(spill))));
            //printf("inserted global store for %s at stmt %d, temp access type %d\n",
            //    tmp_name(spill), ((i_stmt)it_curr(stmtIt))->pos, tmp_getAccess(spill));
        }
    }

    return NULL;
}

//  Add loads for an expression
static i_expr addMemLoadsExpr(structures s, frame frm, iterator stmtIt, i_expr expr) {
    switch(expr->type) {

    case t_TEMP:
        return addLoad(s, frm, stmtIt, expr);

    case t_BINOP:
        expr->u.BINOP.left = addLoad(s, frm, stmtIt, expr->u.BINOP.left);
        expr->u.BINOP.right = addLoad(s, frm, stmtIt, expr->u.BINOP.right);
        return expr;
    
    case t_FCALL:{
        iterator it = it_begin(expr->u.FCALL.args);
        while(it_hasNext(it)) {
            i_expr e = it_next(it);
            it_replace(it, addLoad(s, frm, stmtIt, e));
        }
        it_free(&it);
        return expr;
    }
   
    case t_SYS:
        expr->u.SYS.value = addLoad(s, frm, stmtIt, expr->u.SYS.value);
        return expr;

    case t_CONST:
    case t_NAME:
    case t_MEM:
        return expr;
    
    default:
        assert(0 && "Invalid expr type");
    }
}

