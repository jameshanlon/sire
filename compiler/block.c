#include <stdlib.h>
#include "block.h"
#include "irt.h"
#include "irtprinter.h"
#include "statistics.h"

// A basic block data structure
struct block_ {
    bool mark;
    label start;
    list stmts;
    union {
        struct {
            label a;
            label b;
        } label;
        struct {
            block a;
            block b;
        } block;
    } succ;
};

static list  split(structures, list);
static list  schedule(list);
static void  removeUnreachable(list);
static void  adjustJumps(structures, frame, list);
static void  blockVisitor(block);
static void  markBlocks(list, bool);
static block find(list, label);
static block Block(label, list);

// Main method to compute and schedult basic blocks
void basicBlocks(structures s) {

    // Loop through each process
    iterator it = it_begin(s->ir->procs);
    while(it_hasNext(it)) {
        ir_proc proc = it_next(it);

        //printf("Proc %s\n", frm_name(proc->frm));

        // Spilt into blocks
        list blocks = split(s, proc->stmts.ir);

        //printf("BEFORE:\n");
        //blc_dump(stdout, blocks);

        // Sequence them
        blocks = schedule(blocks);

        // Adjust jumps
        adjustJumps(s, proc->frm, blocks);

        // Remove any uneachable blocks
        removeUnreachable(blocks);

        //printf("AFTER:\n");
        //blc_dump(stdout, blocks);
        
        proc->blocks = blocks;
    }
    it_free(&it);
}

// Basic block: begins with a label, no other labels occur in the block. Any
// JUMP or CJUMP is the last statement in a block.  
list split(structures s, list stmts) {
    
    list blockList = list_New();
    list blockStmts = list_New();
    label start = NULL;
    
    // Add an initial label if there is not one
    if(((i_stmt) list_head(stmts))->type != t_LABEL)
        list_insertFirst(stmts, i_Label(lblMap_NewLabel(s->lbl)));

    iterator stmtsIt = it_begin(stmts);
    i_stmt stmt = it_next(stmtsIt);
    while(it_hasNext(stmtsIt)) {

        // End of a block
        if(stmt->type == t_JUMP || 
           stmt->type == t_CJUMP || 
           stmt->type == t_RETURN) {
            //printf("jump\n");
           
            // Add a new block
            list_add(blockStmts, it_remove(stmtsIt));
            block b = Block(start, blockStmts);
            list_add(blockList, b);
            //printf("added block beginning %s (%d stmts)\n", lbl_name(start),
            //        list_size(blockStmts));

            start = NULL;
            blockStmts = list_New();
            
            // Fill in the successors of the block
            if(stmt->type == t_JUMP) {
                b->succ.label.a = stmt->u.JUMP->u.NAME;
            } 
            else if(stmt->type == t_CJUMP) {
                b->succ.label.a = stmt->u.CJUMP.then->u.NAME;
                b->succ.label.b = stmt->u.CJUMP.other->u.NAME;
            }
            else if(stmt->type == t_RETURN) {
                b->succ.label.a = stmt->u.RETURN.end->u.NAME;
            }
            
            // Check next block begins with a label, if not add one
            stmt = it_next(stmtsIt);
            if(stmt->type != t_LABEL) {
                start = lblMap_NewLabel(s->lbl);
                list_add(blockStmts, i_Label(start));
            }
        }
        // Start of a block
        else if(stmt->type == t_LABEL) {
            //printf("label %s\n", lbl_name(stmt->u.LABEL));
            
            // If found a new label before a jump, add a jump to stmts
            if(start != NULL) {
                //printf("\talready have label\n");
                it_insertBefore(stmtsIt, i_Jump(i_Name(stmt->u.LABEL)));
                stmt = it_prev(stmtsIt);
            }
            // Otherwise, this is a new block
            else {
                start = stmt->u.LABEL;
                list_add(blockStmts, it_remove(stmtsIt));
                stmt = it_next(stmtsIt);
            }
        }
        // Or some other statement 
        else {
            //printf("stmt\n");
            list_add(blockStmts, it_remove(stmtsIt));
            stmt = it_next(stmtsIt);
        }
    }

    // if there wasn't a jump at the end
    if(start!=NULL) {
        list_add(blockStmts, it_remove(stmtsIt));
        block b = Block(start, blockStmts);
        b->succ.label.a = NULL;
        b->succ.label.b = NULL;
        list_add(blockList, b);
    }

    return blockList;
}

// Determine non-overlapping execution traces for a set of basic blocks.  Return
// a flattened list of statements. Also, fill in the block successors as they
// are found, instead of label references.
// The last block remains in its position so is initially marked, then added to
// the traceList at the end
static list schedule(list blocks) {
      
    // Create a duplicate list of blocks
    list blocks_ = list_New();
    list_appendList(blocks_, blocks);

    // Set of non-overlapping traces covering the program
    list traceList = list_New();

    // Initilise block marks
    markBlocks(blocks, false);
    block last = list_tail(blocks_);
    last->mark = true;

    while(!list_empty(blocks_)) {

        // Create a new empty trace
        list trace = list_New();
    
        // Grab the head block
        block b = list_removeFirst(blocks_);
        //printf("block %s =========\n", lbl_name(b->start));

        // While block b is not marked
        while(!b->mark) {
            
            // mark b & append b to the end of the current trace T
            b->mark = true;
            list_add(trace, b);

            // examine the successors of b, if is an unmarked successor b = c
            if(b->succ.label.b != NULL) {
                block succ = find(blocks, b->succ.label.b);
                b->succ.block.b = succ;
                b->succ.block.a = find(blocks, b->succ.label.a);
                //printf("block %s succ by %s\n", lbl_name(b->start), lbl_name(succ->start));
                if(!succ->mark) 
                    b = succ;
            }
            else if(b->succ.label.a != NULL) {
                b->succ.block.b = NULL;
                block succ = find(blocks, b->succ.label.a);
                b->succ.block.a = succ;
                //printf("block %s succ by %s\n", lbl_name(b->start), lbl_name(succ->start));
                if(!succ->mark) 
                    b = succ;
            }
            else b->succ.block.a = NULL;
        }
        
        // all successors of b are marked, end of current trace
        list_appendList(traceList, trace);
    }
      
    // Append the last block
    list_add(traceList, last);

    return traceList;
    //return blocks;
}

// Adjust the blocks:
//    1 Leave any CJUMP followed immediately by its FALSE label
//    2 Negate condition for any CJUMP immediately followed by TRUE
//    3 For any CJUMP followed by neither label we rewrite as:
//       CJUMP(cond, a, b, l_t, l_f) ==>
//           CJUMP(cond, a, b, l_t, l_f)
//           LABEL l'_f
//           JUMP(NAME l_f)
static void adjustJumps(structures s, frame f, list blocks) {

    // For each block and statement
    iterator blockIt = it_begin(blocks);
    while(it_hasNext(blockIt)) {
        block b = it_next(blockIt);
        i_stmt stmt = list_tail(b->stmts);

        // For each CJUMP statement
        if(stmt->type == t_CJUMP) {
            
            block next = it_peekNext(blockIt);
            i_expr trueBranch = stmt->u.CJUMP.then;
            i_expr falseBranch = stmt->u.CJUMP.other;

            // 1: Concatenate blocks
            if(lbl_compare(falseBranch->u.NAME, next->start)) {
                //list_removeFirst(next->stmts);
            } 
            // 2: negate condition
            else if(lbl_compare(trueBranch->u.NAME, next->start)) {
                temp t = frm_addNewTemp(f, t_tmp_local);
                i_stmt e = i_Move(i_Temp(t), i_Binop(i_xor, 
                            i_Const(0xFFFFFFFF), stmt->u.CJUMP.expr));
                iterator it = it_end(b->stmts);
                it_insertAfter(it, e);
                it_free(&it);
                stmt->u.CJUMP.expr = i_Temp(t);
            }
            // 3: rewrite with false JUMP
            else {
                label f = lblMap_NewLabel(s->lbl);
                list_insertLast(b->stmts, i_Jump(i_Name(f)));
                list_insertLast(b->stmts, i_Label(f));
            }
        }
    }
    it_free(&blockIt);
}

// Remove any blocks which cannot be reached by any valid trace, i.e. a path
// staring at the entry block.
static void removeUnreachable(list blocks) {
    markBlocks(blocks, false);
    block b = list_head(blocks);
    blockVisitor(b);

    // Delete any unmarked (unvisited blocks)
    iterator it = it_begin(blocks);
    while(it_hasNext(it)) {
        block b = it_next(it);
        if(!b->mark) {
            it_remove(it);
            stat_numBlocksRemoved++;
        }
    }
    it_free(&it);
}

// Recursively visit basic blocks
static void blockVisitor(block b) {
    //printf("visitng block %s\n", blc_name(b));
    if(!b->mark) {
        b->mark = true;
        if(b->succ.block.a != NULL) {
            //printf("\tvisitng succ a\n");
            blockVisitor(b->succ.block.a);
        }
        if(b->succ.block.b != NULL) {
            //printf("\tvisitng succ b\n");
            blockVisitor(b->succ.block.b);
        }
    }
}

// Convert basic blocks into a single list of statements. Also perform some
// tidying by removing any JUMP immediately followed by label.  (To be performed
// after scheduling and CJUMP adjustements.)
list blc_stmtSeq(list blocks) { list stmts = list_New();

    // For each block, append all stmts to a new list
    iterator it = it_begin(blocks);
    while(it_hasNext(it)) {
        block b = (block) it_next(it);
        
        // Remove any redundant JUMPS
        i_stmt s = list_tail(b->stmts);
        if(s->type == t_JUMP && it_hasNext(it)) {
            label toLab = s->u.JUMP->u.NAME;
            block nextBlc = it_peekNext(it);
            i_stmt firstStmt = list_head(nextBlc->stmts);
            if(firstStmt->type == t_LABEL) {
                if(streq(lbl_name(toLab), lbl_name(firstStmt->u.LABEL)))
                    list_removeLast(b->stmts);
            }
        }

        list_appendList(stmts, b->stmts);
    }
    it_free(&it);

    return stmts;
}

// Label statements in increasing order
void blc_labelStmts(list blocks) {
    iterator blockIt = it_begin(blocks);
    int i = 0;
    while(it_hasNext(blockIt)) {
        block b = (block) it_next(blockIt);
        iterator stmtIt = it_begin(b->stmts);
        while(it_hasNext(stmtIt)) {
            i_stmt s = it_next(stmtIt);
            s->pos = i++;
        }
        it_free(&stmtIt);
    }
    it_free(&blockIt);
}

// Find a block with label l in a list of blocks
static block find(list blocks, label l) {
    assert(l != NULL && "NULL block refernece");
    iterator it = it_begin(blocks);
    while(it_hasNext(it)) {
        block b = (block) it_next(it);
        if(lbl_compare(l, b->start)) {
            it_free(&it);
            return b;
        }
    }
    //printf("label name %s\n", lbl_name(l));
    assert(0 && "No matching block");
    return NULL;
}

// Return the last block; i.e. the one ending with END
block blc_findLastBlock(list blocks) {
    iterator it = it_begin(blocks);
    while(it_hasNext(it)) {
        block b = it_next(it);
        i_stmt s = list_tail(b->stmts);
        if(s->type == t_END) {
            it_free(&it);
            return b;
        }
    }
    assert(0 && "No block with END statement");
    return NULL;
}

// Mark all blocks with a boolean value
static void markBlocks(list blocks, bool v) {
    iterator it = it_begin(blocks);
    while(it_hasNext(it)) {
        block b = it_next(it);
        b->mark = v;
    }
    it_free(&it);
}

// Print the blocks
void blc_dump(FILE *out, list blocks) {
    iterator it = it_begin(blocks);
    while(it_hasNext(it)) {
        block b = it_next(it);
        printRule(out);
        iterator stmtIt = it_begin(b->stmts);
        while(it_hasNext(stmtIt)) {
            i_stmt s = it_next(stmtIt);
            p_stmt(out, 0, s);
        }
        it_free(&stmtIt);
    }
    it_free(&it);
    printRule(out);
}

// =======================================================================
// Block methods
// =======================================================================

// Block constructor
static block Block(label start, list stmts) {
    block b = (block) chkalloc(sizeof(*b));
    b->mark = false;
    b->start = start;
    b->stmts = stmts;
    b->succ.label.a = NULL;
    b->succ.label.b = NULL;
    return b;
}

block blc_getSucc1(block b) {
    return b->succ.block.a;
}

block blc_getSucc2(block b) {
    return b->succ.block.b;
}

string blc_name(block b) {
    return lbl_name(b->start);
}

list blc_stmts(block b) {
    return b->stmts;
}

