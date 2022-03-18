#include "liveness.h"
#include "block.h"
#include "label.h"
#include "set.h"
#include "statistics.h"
#include "irtprinter.h"

#define DEBUG 0 

static void printSets(list blocks, bool);

// Conduct live variable analysis over the set of blocks
void liveness_compute(list blocks) {

    // Initalise def, use, in and out sets
    iterator blockIt = it_begin(blocks);
    while(it_hasNext(blockIt)) {
        block b = it_next(blockIt);
        iterator stmtIt = it_begin(blc_stmts(b));
        while(it_hasNext(stmtIt)) {
            i_stmt stmt = it_next(stmtIt);
            //printf("stmt %d: ", stmt->pos); 
            //p_stmt(stdout, 0, stmt);

            // Delete any old sets
            set_delete(stmt->def);
            set_delete(stmt->use);
            set_delete(stmt->in);
            set_delete(stmt->out);

            // Initialise new sets
            stmt->def = i_getDefSet(stmt);
            stmt->use = i_getUseSet(stmt);
            stmt->in = set_New(&tmp_cmpTemp);
            stmt->out = set_New(&tmp_cmpTemp);
        }
        it_free(&stmtIt);
    }
    it_free(&blockIt);
    //printSets(blocks, true);

    // Iterate in reverse over blocks and statements until in and out sets
    // become stable
    bool done = false;
    int iterations = 0;
    while(!done) {
        done = true;
        iterations++;
        
        // For each block
        blockIt = it_end(blocks);
        while(it_hasPrev(blockIt)) {
            block b = it_prev(blockIt);
        
            // For each statement
            iterator stmtIt = it_end(blc_stmts(b));
            while(it_hasPrev(stmtIt)) {
                i_stmt s = it_prev(stmtIt);
                //printf("stmt %d\n", s->pos);
                
                set in_ = set_copy(s->in); 
                set out_ = set_copy(s->out);
                
                // Last stmt: union of in sets of successors
                set_clear(s->out);
                if(s == list_tail(blc_stmts(b))) {
                    if(blc_getSucc1(b) != NULL) {
                        block succ = blc_getSucc1(b);
                        i_stmt succStmt = list_head(blc_stmts(succ));
                        set_union(s->out, succStmt->in);
                    }
                    if(blc_getSucc2(b) != NULL) {
                        block succ = blc_getSucc2(b);
                        i_stmt succStmt = list_head(blc_stmts(succ));
                        set_union(s->out, succStmt->in);
                    }
                }
                // First and middle stmts
                else {
                    i_stmt succStmt = it_peekNext(stmtIt);
                    set_union(s->out, succStmt->in);
                }

                // in(s) = use(s) U (out(s)-def(s)
                set tmp = set_copy(s->out);
                set_minus(tmp, s->def);
                set_union(tmp, s->use);
                set_replace(s->in, tmp);

                // Check in_(s) == in(s) && out_(s) == out(s)
                if(!set_equal(in_, s->in) || !set_equal(out_, s->out))
                    done = false;

                // Delete temporary sets used
                set_delete(in_);
                set_delete(out_);
                set_delete(tmp);
            }
            it_free(&stmtIt);
        }
        it_free(&blockIt);
        
        //printf("iteration %d\n", iterations);
        //printSets(blocks, true);
    }

    if(DEBUG) {
        printf("Liveness converged in %d iterations\n", iterations);
        //printSets(blocks, false);
        printSets(blocks, true);
    }
}

// Remove redundant statements s: a = b op c where a not in out(s)
void liveness_eliminateDead(list blocks) {

    int i = 0;

    // For each block
    iterator blockIt = it_begin(blocks);
    while(it_hasNext(blockIt)) {
        block b = it_next(blockIt);

        // For each statement
        iterator stmtIt = it_begin(blc_stmts(b));
        while(it_hasNext(stmtIt)) {
            i_stmt s = it_next(stmtIt);

            //assert((set_size(s->def) == 0 || set_size(s->def) == 1)
            //        && "Statement definitions > 1");
            
            if(set_size(s->def) != 0) {
                temp t = list_head(set_elements(s->def));
                if(!set_contains(s->out, tmp_name(t), &tmp_cmpName)) {
                    it_remove(stmtIt);
                    /*printf("Removed stmt %d: ", s->pos); p_stmt(stdout, 0, s);
                    printf("stmt %d: use(%s) def(%s) out(%s) in(%s)\n", s->pos, 
                        set_string(s->use, &tmp_str), set_string(s->def, &tmp_str), 
                        set_string(s->out, &tmp_str), set_string(s->in,
                        &tmp_str));*/
                    i++;
                }
            }

        }
        it_free(&stmtIt);
    }
    it_free(&blockIt);

    // Update stats
    stat_numKilledStmts += i;
}


// Print contents of sets
static void printSets(list blocks, bool all) {
    iterator blockIt = it_begin(blocks);
    int i = 0;
    while(it_hasNext(blockIt)) {
        block b = it_next(blockIt);
        iterator stmtIt = it_begin(blc_stmts(b));
        while(it_hasNext(stmtIt)) {
            i_stmt stmt = it_next(stmtIt);
            if(all) {
                printf("stmt %d: use(%s) def(%s) out(%s) in(%s)\n", i++,
                    set_string(stmt->use, &tmp_str), set_string(stmt->def, &tmp_str), 
                    set_string(stmt->out, &tmp_str), set_string(stmt->in, &tmp_str));
            }
            else {
                printf("stmt %d: out(%s)\n", i++, set_string(stmt->out, &tmp_str));
            }
        }
        it_free(&stmtIt);
    }
    it_free(&blockIt);
}

