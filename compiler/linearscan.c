#include <stdlib.h>
#include "linearscan.h"
#include "block.h"
#include "statistics.h"
#include "irtprinter.h"

#define DEBUG 0 

// Live interval data structure
struct liveInterval_ {
    string  name;
    int     reg;
    int     location;
    int     begin;
    int     end;
    bool    spill;
    t_spill type;
    string  spilled;
};

// LiveInterval object methods
static liveInterval LiveInterval(string, int, bool, string);
static string       liveIntervalStr(void *);
static bool         liveIntervalEq(void *, void *);
static bool         cmpNameRange(void *interval, void *name);
static bool         cmpFormalInterval(void *interval, void *name);
static void         deleteLiveInterval(void *);

// Linear scan methods
static list         computeLiveIntervals(list blocks);
static void         linearScan(frame, list intervals, bool *);
static void         removePreAllocatedParams(frame, list, list);
static void         removePreAllocatedRegs(list, list, list);
static list         removePreAllocated(frame, list stmts, list intervals);
static void         expireOldIntervals(list active, list, list, liveInterval);
static void         addToActive(list active, liveInterval);
static void         spillInterval(frame, liveInterval);
static void         spillAtInterval(frame, list active, liveInterval);
static void         assignRegs(list intervals, list blocks);
static void         addStackLoads(frame, list liveInts, list blocks);
static void         setUsedRegs(frame, list intervals);

// Iteratve register allocation phase
void linScan_compute(frame f, list blocks, list *liveIntervals, bool *spilled) {
    
    // Delete any existing liveInterval list
    if(*liveIntervals != NULL)
        list_deepDelete(*liveIntervals, &deleteLiveInterval);

    // Compute a new set of liveIntervals
    blc_labelStmts(blocks);
    *liveIntervals = computeLiveIntervals(blocks);
    
    // Perform linear scan
    list preAllocated = removePreAllocated(f, blocks, *liveIntervals);
    linearScan(f, *liveIntervals, spilled);
    
    // Add the removed pre-allocated back in
    list_appendList(*liveIntervals, preAllocated);
    list_delete(preAllocated);
    
    if(DEBUG) {
        printf("Live intervals:\n");
        list_dump(*liveIntervals, stdout, &liveIntervalStr); 
    }

    // Assign regsiters to TEMPS based on liveInterval colourings
    assignRegs(*liveIntervals, blocks);
}

// Completion
void linScan_complete(frame f, list liveIntervals, list blocks) {
    addStackLoads(f, liveIntervals, blocks);
    setUsedRegs(f, liveIntervals);
    if(DEBUG) {
        printRule(stdout);
        printf("Variable assignments:\n");
        list_dump(liveIntervals, stdout, &liveIntervalStr);
        printRule(stdout);
    }
}

// LiveInterval constructor. Arguments spill and spilled indicate whether the
// live interval represents some reduced live range of a program variable. It is
// necessary to record this in order to assign the correct formal values from
// the stack into registers for use.
static liveInterval LiveInterval(string name, int begin, bool spill, string spilled) {
    liveInterval r = (liveInterval) chkalloc(sizeof(*r));
    r->name     = name;
    r->reg      = -1;
    r->location = -1;
    r->begin    = begin;
    r->end      = begin + 1; // A liveout-var is live in the next stmt
    r->type     = t_spill_none;
    r->spill    = spill;
    r->spilled  = spilled;
    return r;
}

static string liveIntervalStr(void *p) {
    liveInterval r = (liveInterval) p;
    string spill = r->spill ? StringFmt(" (%s)", r->spilled) : "";
    switch(r->type) {
    case t_spill_none:
        return StringFmt("%s%s [%d:%d] r%d", 
                r->name, spill, r->begin, r->end, r->reg);
    case t_spill_local:
        return StringFmt("%s%s [%d:%d] sp[%d] (local spill)", 
                r->name, spill, r->begin, r->end, r->location);
    case t_spill_caller:
        return StringFmt("%s%s [%d:%d] sp[%d] (caller spill)", 
                r->name, spill, r->begin, r->end, r->location);
    case t_spill_global:
        return StringFmt("%s%s [%d:%d] (global spill)", 
                r->name, spill, r->begin);
    default:
        assert(0 && "Invalid liveInterval type");
    }
}

// Compare a formal name with a liveInterval, where it is a temporary
// performing spilling for the formal variable 'spilled'
/*static bool cmpSpilledFormalInterval(void *interval, void *name) {
    liveInterval i = (liveInterval) interval;
    if(i->spill)
        return streq(name, i->spilled);
    return false;
}*/

// Compare a formal name with a liveInterval
static bool cmpFormalInterval(void *interval, void *name) {
    liveInterval i = (liveInterval) interval;
    return streq(name, i->name);
}

// Compare a name with a liveInterval
static bool cmpNameRange(void *interval, void *name) {
    return streq(name, ((liveInterval) interval)->name);
}

// Compare two live intervals
static bool liveIntervalEq(void *element, void *remove) {
    return element == remove;
}

static void deleteLiveInterval(void *p) {
    free(p);
}

// Compute the live ranges for each variable
static list computeLiveIntervals(list blocks) {

    //printf("Computing live intervals...\n");
    list intervals = list_New();

    // For each block
    iterator blockIt = it_begin(blocks);
    while(it_hasNext(blockIt)) {
        block b = it_next(blockIt);

        // For each statement
        iterator stmtIt = it_begin(blc_stmts(b));
        while(it_hasNext(stmtIt)) {
        
            i_stmt s = it_next(stmtIt);
            //printf("Statement %d:", s->pos); p_stmt(stdout, 0, s);

            // Check if any new variables have become active
            iterator liveIt = it_begin(set_elements(s->out));
            while(it_hasNext(liveIt)) {
                temp t = (temp) it_next(liveIt);
                if(!list_contains(intervals, tmp_name(t), &cmpNameRange)) {

                    liveInterval r = LiveInterval(tmp_name(t), s->pos, 
                            tmp_spill(t), tmp_spilled(t));
                    list_add(intervals, r);
                    //printf("\t%s active at %d\n", tmp_name(t), s->pos);
                }
                else {
                    liveInterval r = list_getFirst(intervals, tmp_name(t),
                            &cmpNameRange);
                    r->end = s->pos + 1;
                    //printf("\t%s inactive at %d\n", tmp_name(t), s->pos);
                }
            }
            it_free(&liveIt);
        }
        it_free(&stmtIt);
    }
    it_free(&blockIt);
   
    //list_dump(intervals, stdout, &liveIntervalStr);
    return intervals;
}

// Remove all parameter arguments from live intervals and add them to a pre
// allocated list
static void removePreAllocatedParams(frame f, list intervals, list preAllocated) {
    
    list formalAccesses = frm_formalAccesses(f);
    iterator it = it_begin(formalAccesses);
    while(it_hasNext(it)) {
        frm_access a = it_next(it);
        if(frm_access_inReg(a)) {
            //printf("\tremoving %s..\n", frm_access_name(a));
            liveInterval i = list_remove(intervals, frm_access_name(a), &cmpNameRange);

            // If interval NULL, varaible has no live range, i.e. not used
            if(i != NULL) {
                i->reg = frm_access_reg(a);
                list_add(preAllocated, i);
            }
            //printf("removed %s from intervals\n", frm_access_name(a));
        }
    } 
    it_free(&it);
}

// Remove all pre-allocated registers and add them to a pre-allocated list
static void removePreAllocatedRegs(list blocks, list intervals, 
        list preAllocated) {

    // Remove any TEMPS preallocated to r11
    iterator blockIt = it_begin(blocks);
    while(it_hasNext(blockIt)) {
        block b = it_next(blockIt);

        iterator stmtIt = it_begin(blc_stmts(b));
        while(it_hasNext(stmtIt)) {
            i_stmt s = it_next(stmtIt);

            // Look for any load word address from cp, i.e. temp = &cp[i] and
            // pre-allocate r11 to the temp used
            if(s->type == t_MOVE) {
                i_expr dst = s->u.MOVE.dst;
                i_expr src = s->u.MOVE.src;
                if(dst->type == t_TEMP && src->type == t_MEM) {
                    if(src->u.MEM.type == t_mem_cpa) {
                        string name = tmp_name(dst->u.TEMP);
                        liveInterval i = list_remove(intervals, 
                                name, &cmpNameRange);
                        assert(i != NULL && "live interval not found");
                        i->reg = 11;
                        list_add(preAllocated, i);
                    }
                }
            }
        }
        it_free(&stmtIt);
    }
    it_free(&blockIt);
}

// Remove all live ranges with pre-allocated register assignments
static list removePreAllocated(frame f, list blocks, list intervals) {
    
    list preAllocated = list_New();
    removePreAllocatedParams(f, intervals, preAllocated);
    removePreAllocatedRegs(blocks, intervals, preAllocated);
    return preAllocated;
}

// Expire any intervals ending before the current interval
static void expireOldIntervals(list active, list inactive, list regs, liveInterval i) {
    iterator it = it_begin(active);
    while(it_hasNext(it)) {
        liveInterval j = it_next(it);
        
        if(j->end >= i->begin) {
            it_free(&it);
            return;
        }

        //printf("\tExpired %s from active\n", j->name);
        it_remove(it);
        list_insertFirst(regs, reg_Reg(j->reg));
        list_add(inactive, j);
    }
    it_free(&it);
}

// Insert a new live interval into active, preserving ordering by end points
static void addToActive(list active, liveInterval i) {
    iterator it = it_begin(active);
    while(it_hasNext(it)) {
        liveInterval j = it_next(it);
        if(i->end <= j->end) {
            it_insertBefore(it, i);
            it_free(&it);
            //printf("\tadded %s to active with r%d\n", i->name, i->reg);
            return;
        }
    }
    it_free(&it);
    list_insertLast(active, i);
    //printf("\tadded %s to active with r%d\n", i->name, i->reg);
}

// Evaluate the location of the spill and update accordingly
static void spillInterval(frame f, liveInterval spill) {
    spill->type = frm_spillType(f, spill->name); 
    switch(spill->type) {
    case t_spill_local:
        spill->location = frm_allocLocal(f, spill->name);
        break;
    case t_spill_caller:
        spill->location = frm_formalLocation(f, spill->name);
        break;
    case t_spill_global:
        spill->location = -1;
        break;
    default:
        assert(0 && "Invalid liveInterval type");
    }

    stat_numSpiltVars++;
}

// Spill a variable onto the stack: either last active interval or the current one
static void spillAtInterval(frame f, list active, liveInterval i) {
  
    liveInterval spill = list_tail(active);

    if(spill->end > i->end) {
        //printf("\tSpilled %s from active\n", spill->name);
        i->reg = spill->reg;
        spill->reg = -1;
        spillInterval(f, spill);
        list_remove(active, spill, &liveIntervalEq);
        addToActive(active, i);
    }
    else {
        //printf("\tSpilled %s\n", i->name);
        spillInterval(f, i);
    }
}

// Perform a linear-scan allocation of regsiters by analysing live-ranges. See
// "Linear scan register allocation, Poletto & Sarkar, 1999"
static void linearScan(frame frm, list intervals, bool *spilled) {

    //printf("Linear scan...\n");
    
    //printf("Intervals:\n");
    //list_dump(intervals, stdout, &liveIntervalStr);

    // Get the set of available registers
    list regs = frm_regSet(frm);
    int avalRegs = list_size(regs);

    // Current active live intervals, sorted in order of increasing end point
    list active = list_New();
    list inactive = list_New();

    // Iterate over the live intervals
    iterator it = it_begin(intervals);
    while(it_hasNext(it)) {
        liveInterval i = it_next(it);
        //printf("interval: %s from %d to %d. NumActive=%d\n", 
        //        i->name, i->begin, i->end, list_size(active));
        expireOldIntervals(active, inactive, regs, i);
        if(list_size(active) == avalRegs) {
            spillAtInterval(frm, active, i);
            *spilled = true;
        }
        else {
            //list_dump(regs, stdout, &regStr);
            int *reg = list_removeFirst(regs); 
            i->reg = *reg;
            //printf("\tAssigned %s to reg %d. Num regs avail %d\n", 
            //   i->name, i->reg, list_size(regs));
            addToActive(active, i);
        }
    }
    it_free(&it);

    list_delete(regs);
    list_delete(active);
    list_delete(inactive);
}

// Assign regsiters to temps according to interval allocations
static void assignRegs(list intervals, list blocks) {

    //printf("Assigning registers..\n");

    // take union of use and def sets
    iterator blockIt = it_begin(blocks);
    while(it_hasNext(blockIt)) {
        block b = it_next(blockIt);
        iterator stmtIt = it_begin(blc_stmts(b));
        while(it_hasNext(stmtIt)) {
            i_stmt s = it_next(stmtIt);
            set_append(s->def, s->use);
        }
        it_free(&stmtIt);
    }
    it_free(&blockIt);

    // For each interval
    iterator it = it_begin(intervals);
    while(it_hasNext(it)) {
        liveInterval r = it_next(it);

        // For each block
        iterator blockIt = it_begin(blocks);
        while(it_hasNext(blockIt)) {
            block b = it_next(blockIt);

            // For each statement
            iterator stmtIt = it_begin(blc_stmts(b));
            while(it_hasNext(stmtIt)) {
                i_stmt s = it_next(stmtIt);
                
                if(s->pos > r->end + 1) break;
               
                // If statement in interval
                // NOTE: r->end+1 for next live-in stmt
                if(s->pos >= r->begin && s->pos <= (r->end + 1)) {

                    /*printf("\t%s to temps in stmt %d: def(%s), use(%s)\n", 
                            r->name, s->pos, set_string(s->def, &tmp_str),
                            set_string(s->use, &tmp_str));*/
                    list temps = (list) set_get(s->def, r->name, &tmp_cmpName);

                    // If it's not an argument
                    if(temps != NULL) {

                        iterator tempIt = it_begin(temps);
                        while(it_hasNext(tempIt)) {
                            temp t = it_next(tempIt);

                            // Set the register, or if it has been spilled, the
                            // location and a flag in temp to record the
                            // original variable name
                            switch(r->type) {
                            case t_spill_none:
                                tmp_setRegAccess(t, r->reg);
                                break;
                            case t_spill_local: 
                                tmp_setFrameAccess(t, r->location);
                                tmp_setSpilled(t, tmp_name(t));
                                break;
                            case t_spill_caller:
                                tmp_setCallerAccess(t, r->location);
                                tmp_setSpilled(t, tmp_name(t));
                                break;
                            case t_spill_global:
                                tmp_setDataAccess(t);
                                tmp_setSpilled(t, tmp_name(t));
                                break;
                            default: 
                                assert(0 && "Invalid liveInterval type");
                            }
                            
                            //printf("\tassigned %s to stmt %d type %d:  {%s}\n", r->name,
                            //        s->pos, tmp_getAccess(t), set_string(s->def, &tmp_str));
                        }
                        it_free(&tempIt);
                        list_delete(temps);
                    }
                }
            }
            it_free(&stmtIt);
        }
        it_free(&blockIt);
    }
}

// Add loads from the stack to live variables, at the beginning of their range
static void addStackLoads(frame f, list liveInts, list blocks) {

    iterator it = it_begin(frm_formalAccesses(f));
    while(it_hasNext(it)) {
        frm_access a = it_next(it);

        // If it's in the previous stack frame, load in the value
        if(frm_access_inFrame(a)) {

            // Look for the first non-spill interval for the formal
            // (as that will load it from memory anyway)
            liveInterval r = list_getFirst(liveInts, 
                    frm_access_name(a), &cmpFormalInterval);

            if(r != NULL) {

                // Add a mem load from location to first live interval using it
                temp t = frm_addTemp(f, r->name, t_tmp_local);
                tmp_setRegAccess(t, r->reg); 
                i_stmt stmt = i_Move(i_Temp(t), 
                        i_Mem(t_mem_spi, NULL, i_Const(frm_access_off(a))));
               
                // Insert it at the beginning of the live range
                bool done = false;
                iterator blockIt = it_begin(blocks);
                while(it_hasNext(blockIt)) {
                    block b = it_next(blockIt);
                    iterator stmtIt = it_begin(blc_stmts(b));
                    while(it_hasNext(stmtIt)) {
                        i_stmt s = it_next(stmtIt);
                        if(s->pos == r->begin) {
                            it_insertBefore(stmtIt, stmt);
                            done = true;
                            //printf("inserted stack arg load for %s at %d\n",
                            //        r->name, s->pos);
                            break;
                        }
                    }
                    it_free(&stmtIt);
                    if(done) break;
                }
                it_free(&blockIt);
            }
        }
    }
    it_free(&it);
}

// Obtain a list of used registers from final assignments to live intervals, and
// add this to the frame
static void setUsedRegs(frame f, list intervals) {
    list usedRegs = list_New();
    iterator it = it_begin(intervals);
    while(it_hasNext(it)) {
        liveInterval i = it_next(it);
        if(!list_contains(usedRegs, &(i->reg), &reg_cmp))
            list_add(usedRegs, reg_Reg(i->reg));
    }
    it_free(&it);
    frm_setUsedRegs(f, usedRegs);
    //list_dump(usedRegs, stdout, &showReg);
    list_deepDelete(usedRegs, &reg_delete);
}

