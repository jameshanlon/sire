#include "regalloc.h"
#include "block.h"
#include "liveness.h"
#include "linearscan.h"
#include "spill.h"

// Register allocation:
//
//    a. Repeat until (no variables spilled):
//        1. Compute liveness
//        2. Eliminate dead statements
//        3. Perform linear scan
//        5. Assign registers to TEMPS in statements based on live interval
//           allocations.
//        6. For each spilled variable, add necessary loads and stores after uses and
//          defs.
//    b. Add any necessary stack loads into registers, immediately before the live
//    range of a variable.
void allocRegs(structures s) {
    
    // Loop through each procedure
    iterator it = it_begin(s->ir->procs);
    while(it_hasNext(it)) {
        ir_proc proc = it_next(it);
        //printf("Regalloc: proc %s\n", frm_name(proc->frm));
   
        // Perform register allocation
        bool spilled;
        list liveIntervals = NULL;

        do {
            //printf("New iteration\n");

            // Liveness analysis & dead-code elimination
            liveness_compute(proc->blocks);
            liveness_eliminateDead(proc->blocks);

            // Linear scan
            spilled = false;
            linScan_compute(proc->frm, proc->blocks, &liveIntervals, &spilled);
            
            // Add in loads/stores for any variables in memory
            spill_rewrite(s, proc->frm, proc->blocks);
        }
        while(spilled);

        // Complete by adding any loads from stack and updating used regs in frame
        linScan_complete(proc->frm, liveIntervals, proc->blocks);
        
        // Flatten blocks into list stmts
        proc->stmts.ir = blc_stmtSeq(proc->blocks);
    }
    it_free(&it);
}
