#include "error.h"
#include "../include/definitions.h"
#include "temp.h"
#include "label.h"
#include "translate.h"

static void   buildChildren(structures s);

static list   body        (structures, frame, a_stmt);

// Statement translations
static void   stmt        (structures, frame, a_stmt, list);
static void   stmt_return (structures, frame, a_stmt, list);
static void   stmt_if     (structures, frame, a_stmt, list);
static void   stmt_while  (structures, frame, a_stmt, list);
static void   stmt_for    (structures, frame, a_stmt, list);
static void   stmt_pCall  (structures, frame, a_stmt, list);
static void   stmt_ass    (structures, frame, a_stmt, list);
static void   stmt_input  (structures, frame, a_stmt, list);
static void   stmt_output (structures, frame, a_stmt, list);
static void   stmt_seq    (structures, frame, a_stmtSeq, list);
static void   stmt_par    (structures, frame, a_stmtPar, list);
static void   stmt_on     (structures, frame, a_stmt, list);
static void   stmt_alias  (structures, frame, a_stmt, list);
static void   stmt_connect(structures, frame, a_stmt, list);

// Expression translations
static i_expr expr        (structures, frame, a_expr, list);
static i_expr expr_monadic(structures, frame, a_expr, list);
static i_expr expr_diadic (structures, frame, a_expr, list);
static void   argList     (structures, frame, a_exprList, list args, list stmts);

// Element translations
static i_expr elem        (structures, frame, a_elem, list);
static i_expr elem_name   (structures, frame, a_elem);
static i_expr elem_sub    (structures, frame, a_elem, list);
static i_expr elem_FCall  (structures, frame, a_elem, list);
static i_expr elem_number (a_elem);
static i_expr elem_boolean(a_elem);
static i_expr elem_string (a_elem);

// Main method
void translate(structures s) {
   
    // Translate each procedure body
    iterator it = it_begin(s->ir->procs);
    while(it_hasNext(it)) {
        ir_proc p = it_next(it);
        p->stmts.ir = body(s, p->frm, p->stmts.as);
    }
    it_free(&it);

    buildChildren(s);
    //dumpChildren(s, stdout);
}

static void buildChildren(structures s) {
    
    // Build full child call lists for each function
    bool change;
    do {
        change = false;

        // For each procedure
        iterator it = it_begin(s->ir->procs);
        while(it_hasNext(it)) {
            ir_proc p = it_next(it);
        
            // For each child call
            iterator childIt = it_begin(ir_childCalls(p));
            while(it_hasNext(childIt)) {
                ir_proc child = it_next(childIt);

                // For all of it's children (grandchildren)
                iterator grandchildIt = it_begin(ir_childCalls(child));
                while(it_hasNext(grandchildIt)) {
                    ir_proc grandchild = it_next(grandchildIt);
    
                    // Add them if they don't already exist
                    if(!list_contains(ir_childCalls(p), 
                                frm_name(grandchild->frm), &isNamedProc)) {
                        list_add(ir_childCalls(p), grandchild);
                        change = true;
                    }

                }
                it_free(&grandchildIt);
            }
            it_free(&childIt);
        }
        it_free(&it);
    }
    while(change);
}

// Dump the list of children to a file
void dumpChildren(structures s, FILE *out) {
    
    // For each procedure
    iterator it = it_begin(s->ir->procs);
    while(it_hasNext(it)) {
        ir_proc parent = it_next(it);
                
            fprintf(out, "%s:\n", frm_name(parent->frm));
            
            // For each child call
            iterator childIt = it_begin(ir_childCalls(parent));
            while(it_hasNext(childIt)) {
                ir_proc child = it_next(childIt);
                
                fprintf(out, "\t%s\n", frm_name(child->frm));
            }
            it_free(&childIt);
    }
    it_free(&it);
        
}

// Translate a procedure body
static list body(structures s, frame f, a_stmt p) {
    list stmts = list_New();
    label epilogue = lblMap_NewLabel(s->lbl);
    frm_setEpilogueLbl(f, epilogue);
    stmt(s, f, p, stmts);
    list_add(stmts, i_Label(epilogue));
    list_add(stmts, i_End());
    return stmts;
}

//========================================================================
// Statements
//========================================================================

// Stmt
static void stmt(structures s, frame f, a_stmt p, list l) {
    switch(p->type) {
    case t_stmt_skip:     i_Nop();                         break;
    case t_stmt_return:   stmt_return (s, f, p, l);        break;
    case t_stmt_if:       stmt_if     (s, f, p, l);        break;
    case t_stmt_while:    stmt_while  (s, f, p, l);        break;
    case t_stmt_for:      stmt_for    (s, f, p, l);        break;
    case t_stmt_pCall:    stmt_pCall  (s, f, p, l);        break;
    case t_stmt_ass:      stmt_ass    (s, f, p, l);        break;
    case t_stmt_input:    stmt_input  (s, f, p, l);        break;
    case t_stmt_output:   stmt_output (s, f, p, l);        break;
    case t_stmt_seq:      stmt_seq    (s, f, p->u.seq, l); break;
    case t_stmt_par:      stmt_par    (s, f, p->u.par, l); break;
    case t_stmt_on:       stmt_on     (s, f, p, l);        break;
    case t_stmt_connect:  stmt_connect(s, f, p, l);        break;
    case t_stmt_alias:    stmt_alias  (s, f, p, l);        break;
    default: assert(0 && "Invalid stmt type");
    }
}

// Return statement
static void stmt_return(structures s, frame f, a_stmt p, list stmts) {
    label l = lblMap_getNamed(s->lbl, lbl_name(frm_getEpilogueLbl(f)));
    i_stmt r = i_Return(i_Name(l), expr(s, f, p->u.return_.expr, stmts));
    list_add(stmts, r);
}

// If statement
// ==================================
// IF expr THEN stmt ELSE stmt
// ==================================
//   CJUMP cond label-then label-else
//   LABEL then
//     stmts
//     JUMP end
//   LABEL else
//     stmts
//   LABEL end
static void stmt_if(structures s, frame f, a_stmt p, list stmts) {
    label lThen = lblMap_NewLabel(s->lbl);
    label lElse = lblMap_NewLabel(s->lbl);
    label lEnd = lblMap_NewLabel(s->lbl);
   
    // Lift any non-temp elements from condition
    i_expr cond = expr(s, f, p->u.if_.expr, stmts);
    if(cond->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), cond));
        cond = i_Temp(t);
    }

    // Construct the statement
    list_add(stmts, i_CJump(cond, i_Name(lThen), i_Name(lElse)));
    list_add(stmts, i_Label(lThen));
    stmt(s, f, p->u.if_.stmt1, stmts);
    list_add(stmts, i_Jump(i_Name(lEnd)));
    list_add(stmts, i_Label(lElse));
    stmt(s, f, p->u.if_.stmt2, stmts);
    list_add(stmts, i_Label(lEnd));
}

// While statement
// ============================
// WHILE expr DO stmt
// ============================
//   LABEL start
//   CJUMP label-then label-end
//   LABEL then
//     stmts
//     JUMP start
//   LABEL end
static void stmt_while(structures s, frame f, a_stmt p, list stmts) {   

    label lStart = lblMap_NewLabel(s->lbl);
    label lThen = lblMap_NewLabel(s->lbl);
    label lEnd = lblMap_NewLabel(s->lbl);
    
    list_add(stmts, i_Label(lStart));
    
    // Lift any non-temp elements from condition
    i_expr cond = expr(s, f, p->u.while_.expr, stmts);
    if(cond->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), cond));
        cond = i_Temp(t);
    }

    // Construct the statement
    list_add(stmts, i_CJump(cond, i_Name(lThen), i_Name(lEnd)));
    list_add(stmts, i_Label(lThen));
    stmt(s, f, p->u.while_.stmt, stmts);
    list_add(stmts, i_Jump(i_Name(lStart)));
    list_add(stmts, i_Label(lEnd));
}

// For statement
// ===================================
//   FOR elem ASS expr TO expr DO stmt
// ===================================
//   pre-condition stmt
//   var := precondition
//   LABEL start
//   tmp := var < post-condition 
//   CJUMP tmp label-then label-end
//   LABEL then
//      stmts
//      var := var + 1
//      JUMP start
//   LABEL end
static void stmt_for(structures s, frame f, a_stmt p, list stmts) {

    label lStart = lblMap_NewLabel(s->lbl);
    label lThen = lblMap_NewLabel(s->lbl);
    label lEnd = lblMap_NewLabel(s->lbl);
  
    // Add the precondition assignment and start label
    string name = p->u.for_.var->u.name->name;
    temp varTmp = frm_addTemp(f, name, t_tmp_local);
    i_expr preCond = expr(s, f, p->u.for_.pre, stmts);
    list_add(stmts, i_Move(i_Temp(varTmp), preCond));
    list_add(stmts, i_Label(lStart));
    
    // Create the post-condition expr and lift any non-temp elements from condition
    i_expr postCond = expr(s, f, p->u.for_.post, stmts);
    if(postCond->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), postCond));
        postCond = i_Temp(t);
    }

    // Add in cond: var <= postCond
    temp condTmp = frm_addNewTemp(f, t_tmp_local);
    i_expr cond = i_Binop(i_le, i_Temp(varTmp), postCond);
    list_add(stmts, i_Move(i_Temp(condTmp), cond));

    // Construct the CJUMP, label and body
    list_add(stmts, i_CJump(i_Temp(condTmp), i_Name(lThen), i_Name(lEnd)));
    list_add(stmts, i_Label(lThen));
    stmt(s, f, p->u.for_.stmt, stmts);

    // Close the loop: increment, jump and end
    list_add(stmts, i_Move(i_Temp(varTmp), 
                i_Binop(i_plus, i_Temp(varTmp), i_Const(1))));
    list_add(stmts, i_Jump(i_Name(lStart)));
    list_add(stmts, i_Label(lEnd));
}

// Procedure call statement
static void stmt_pCall(structures s, frame f, a_stmt p, list stmts) {
    
    label l = lblMap_getNamed(s->lbl, p->u.pCall.name->name);
    list args = list_New();
    argList(s, f, p->u.pCall.exprList, args, stmts);
    list_add(stmts, i_PCall(i_Name(l), args));

    // Add this child call to the frame's list
    ir_addChildCall(s->ir, frm_name(f), lbl_name(l));
}

// Assignment statement
static void stmt_ass(structures s, frame f, a_stmt p, list stmts) {
    
    i_expr dst = elem(s, f, p->u.ass.dst, stmts);
    i_expr src = expr(s, f, p->u.ass.src, stmts);

    // If dest is MEM, src must be TEMP (or CONST?)
    if(dst->type == t_MEM) {
        if(src->type != t_TEMP) {
            temp t = frm_addNewTemp(f, t_tmp_local);
            list_add(stmts, i_Move(i_Temp(t), src));
            src = i_Temp(t);
        }
    }

    list_add(stmts, i_Move(dst, src));
}

// Input statement
static void stmt_input(structures s, frame f, a_stmt p, list stmts) {
    
    i_expr dst = expr(s, f, p->u.io.src, stmts);
    i_expr src = elem(s, f, p->u.io.dst, stmts);

    // If src not TEMP, lift above
    if(src->type != t_TEMP && src->type != t_SYS) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), src));
        src = i_Temp(t);
    }

    // If dst not TEMP, lift below
    if(dst->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), i_Temp(t)));
    }
    
    list_add(stmts, i_Input(dst, src));
}

// Output statement
static void stmt_output(structures s, frame f, a_stmt p, list stmts) {
    
    i_expr dst = elem(s, f, p->u.io.dst, stmts);
    i_expr src = expr(s, f, p->u.io.src, stmts);
    
    // If dst not TEMP, lift above
    if(dst->type != t_TEMP && dst->type != t_SYS) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), dst));
        dst = i_Temp(t);
    }

    // If src not TEMP, lift above
    if(src->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), src));
        src = i_Temp(t);
    }

    list_add(stmts, i_Output(dst, src));
}

// On statement
static void stmt_on(structures s, frame f, a_stmt p, list stmts) {
    
    list args = list_New();
    a_elem pCall = p->u.on.pCall;
    label l = lblMap_getNamed(s->lbl, pCall->u.call.name->name);
    i_expr dest = elem(s, f, p->u.on.dest, stmts);

    assert(dest->type==t_SYS && "on destination not SYS");

    // Flatten the arg list
    argList(s, f, pCall->u.call.exprList, args, stmts);
    i_expr pCallExpr = i_FCall(i_Name(l), args);

    // Add the statement
    list_add(stmts, i_On(dest, pCallExpr));
    
    // Add the call to the list of children
    ir_addChildCall(s->ir, frm_name(f), lbl_name(l));
}

// Alias statement
static void stmt_alias(structures s, frame f, a_stmt p, list stmts) {
   
    i_expr dst = elem(s, f, p->u.alias.dst, stmts);
    i_expr src = elem(s, f, p->u.alias.array, stmts);
    i_expr index = expr(s, f, p->u.alias.index, stmts);

    // If src not TEMP, lift out
    if(src->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), src));
        src = i_Temp(t);
    }

    // Multiply index by 4
    if(index->type == t_CONST) 
        index = i_Const(index->u.CONST * 4);
    else {
        // If index is an expression, then lift it out
        if(index->type != t_TEMP) {
            temp t = frm_addNewTemp(f, t_tmp_local);
            list_add(stmts, i_Move(i_Temp(t), index));
            index = i_Temp(t);
        }
        list_add(stmts, i_Move(dst, i_Binop(i_mult, index, i_Const(4))));
        index = dst;
    }

    // Add an assignment to the destination variable of the absoloute
    list_add(stmts, i_Move(dst, i_Binop(i_plus, src, index)));
}

// Connect statement
static void stmt_connect(structures s, frame f, a_stmt p, list stmts) {
   
    i_expr to = elem(s, f, p->u.connect.to, stmts);
    i_expr c1 = elem(s, f, p->u.connect.c1, stmts);
    i_expr c2 = elem(s, f, p->u.connect.c2, stmts);

    assert(to->type==t_SYS && "connect to not SYS");
    assert(c1->type==t_SYS && "connect c1 not SYS");
    assert(c2->type==t_SYS && "connect c2 not SYS");

    // Lift any non-TEMP (i.e. CONST) values out
    if(to->u.SYS.value->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), to->u.SYS.value));
        to->u.SYS.value = i_Temp(t);
    }
    if(c1->u.SYS.value->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), c1->u.SYS.value));
        c1->u.SYS.value = i_Temp(t);
    }
    if(c2->u.SYS.value->type != t_TEMP) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), c2->u.SYS.value));
        c2->u.SYS.value = i_Temp(t);
    }

    // Add an assignment to the destination variable of the absoloute
    list_add(stmts, i_Connect(to, c1, c2));
}

//========================================================================
// Expressions
//========================================================================

// Expression
static i_expr expr(structures s, frame f, a_expr p, list stmts) {
    assert(p != NULL && "a_expr NULL");

    switch(p->type) {
    
    case t_expr_none:   case t_expr_neg:   case t_expr_not:
        return expr_monadic(s, f, p, stmts);

    case t_expr_plus:   case t_expr_minus: case t_expr_mult:
    case t_expr_div:    case t_expr_rem:   case t_expr_or:
    case t_expr_and:    case t_expr_xor:   case t_expr_lshift:
    case t_expr_rshift: case t_expr_eq:    case t_expr_ne:
    case t_expr_ls:     case t_expr_le:    case t_expr_gr:
    case t_expr_ge:
        return expr_diadic(s, f, p, stmts);

    default: assert(0 && "Invalid diadic i_expr type");
    }
}

// Monadic expression
static i_expr expr_monadic(structures s, frame f, a_expr p, list stmts) {

    i_expr element = elem(s, f, p->u.monadic.elem, stmts);

    // If it is a constant value we can evaluate now
    if(element->type == t_CONST) {
        switch(p->type) { 
        case t_expr_none:                                      break; 
        case t_expr_neg: element->u.CONST = -element->u.CONST; break;
        case t_expr_not: element->u.CONST = ~element->u.CONST; break;
        default: assert(0 && "Invalid monadic i_expr type");
        }
        return element;
    }
    // Otherwise a return binop if appropriate
    else {
        switch(p->type) {
        case t_expr_none: return element;
        case t_expr_neg:  return i_Binop(i_minus, i_Const(0), element);
        case t_expr_not:  return i_Binop(i_xor, i_Const(ALLONES),  element);
        default: assert(0 && "Invalid monadic i_expr type");
        }
    }
}

// Diadic expression
static i_expr expr_diadic(structures s, frame f, a_expr p, list stmts) {

    i_expr left = elem(s, f, p->u.diadic.elem, stmts);
    i_expr right = expr(s, f, p->u.diadic.expr, stmts); 

    // If to constant value operands, evaluate them now
    if(left->type == t_CONST && right->type == t_CONST) {
        i_deleteExpr(left);
        i_deleteExpr(right);
        return i_Const(trl_evalOp(p->type, left->u.CONST, right->u.CONST));
    }

    // Lift any left expr not TEMP, NAME or CONST
    if(left->type!=t_TEMP && left->type!=t_NAME && left->type!=t_CONST) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), left));
        left = i_Temp(t);
    }

    // Lift any right expr not TEMP, NAME or CONST
    if(right->type!=t_TEMP && right->type!=t_NAME && right->type!=t_CONST) {
        temp t = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t), right));
        right = i_Temp(t);
    }

    // Otherwise create a BINOP node
    t_binop op;
    switch(p->type) {
    case t_expr_plus:   op = i_plus;   break;
    case t_expr_minus:  op = i_minus;  break;
    case t_expr_mult:   op = i_mult;   break;
    case t_expr_div:    op = i_div;    break;
    case t_expr_rem:    op = i_rem;    break;
    case t_expr_or:     op = i_or;     break;
    case t_expr_and:    op = i_and;    break;
    case t_expr_xor:    op = i_xor;    break;
    case t_expr_lshift: op = i_lshift; break;
    case t_expr_rshift: op = i_rshift; break;
    case t_expr_eq:     op = i_eq;     break;
    case t_expr_ne:     op = i_ne;     break;
    case t_expr_ls:     op = i_ls;     break;
    case t_expr_le:     op = i_le;     break;
    case t_expr_gr:     op = i_gr;     break;
    case t_expr_ge:     op = i_ge;     break;
    default: assert(0 && "Invalid i_BINOP type");
    }

    return i_Binop(op, left, right);
}

//========================================================================
// Elements
//========================================================================

// Element 
//  - Mark temps as global if symbol not defined in scope
static i_expr elem(structures s, frame f, a_elem p, list stmts) {
    assert(p != NULL && "a_elem NULL");

    switch(p->type) {
    case t_elem_name:    return elem_name(s, f, p);
    case t_elem_sub:     return elem_sub(s, f, p, stmts);
    case t_elem_fCall:   return elem_FCall(s, f, p, stmts);
    case t_elem_number:  return elem_number(p); 
    case t_elem_boolean: return elem_boolean(p);
    case t_elem_string:  return elem_string(p);
    case t_elem_expr:    return expr(s, f, p->u.expr, stmts);
    default: 
        assert(0 && "Invalid a_elem type");
        return NULL;
    }
}

// Variable value element
static i_expr elem_name(structures s, frame f, a_elem p) {
    
    string name = p->u.name->name;
    i_expr off, base;
    
    switch(sym_type(p->sym)) {
    
    // For vals return a CONST
    case t_sym_const:
        return i_Const(sym_constVal(p->sym)); 
   
    // For ports return a CONST in the gloabl scope, of a TEMP otherwise
    case t_sym_port: 
        switch(sym_scope(p->sym)) {
        case t_scope_module:
            return i_Const(sym_constVal(p->sym));
        case t_scope_proc:
        case t_scope_func:
            return i_Temp(frm_addTemp(f, name, t_tmp_local));
        default: 
            assert(0 && "invalid port scope");
        }
    
    // Everything else a TEMP
    case t_sym_int:  case t_sym_proc:    case t_sym_func: 
    case t_sym_chan: case t_sym_chanend: {
        t_temp type = sym_scope(p->sym) == t_scope_module ? 
            t_tmp_global : t_tmp_local;
        temp t = frm_addTemp(f, name, type);
        return i_Temp(t);
    }

    // For array variables, load the address they represent
    case t_sym_intArray:
    case t_sym_chanArray: {
        switch(sym_scope(p->sym)) {
        case t_scope_proc:
        case t_scope_func:
            off = i_Const(frm_arrayOffset(f, name));
            return i_Mem(t_mem_spa, NULL, off);

        case t_scope_module:
            base = i_Name(ir_dpLoc(s->ir, name));
            return i_Mem(t_mem_dpa, base, i_Const(0));
        default:
            assert(0 && "invalid scope");
        }
    }

    // For array aliases, load the pointer value
    case t_sym_intAlias: /*case t_sym_chanAlias:*/ {
        t_temp type = sym_scope(p->sym) == t_scope_module ? 
            t_tmp_global : t_tmp_local;
        temp t = frm_addTemp(f, name, type);
        return i_Temp(t);
    }

    // For (local, formal) array references, load the pointer value
    case t_sym_intArrayRef:
    //case t_sym_chanArrayRef:
        switch(sym_scope(p->sym)) {
        case t_scope_proc:
        case t_scope_func:
            return i_Temp(frm_addTemp(f, name, t_tmp_local));
        case t_scope_module:
        default:
            assert(0 && "invalid scope");
        }
        return NULL;

    case t_sym_undefined:
        assert(0 && "undefined name element symbol type");

    default:
        //printf("name sym: %s\n", sym_name(p->sym));
        assert(0 && "invalid name element symbol type");
    }
}

// Array subscript element
static i_expr elem_sub(structures s, frame f, a_elem p, list stmts) {
    
    string name = p->u.sub.name->name;
    i_expr offset = expr(s, f, p->u.sub.expr, stmts);
    i_expr base = NULL;

    // Sort out the offset
    switch(offset->type) {
    case t_BINOP:
    case t_FCALL: {
        temp t1 = frm_addNewTemp(f, t_tmp_local);
        list_add(stmts, i_Move(i_Temp(t1), offset));
        offset = i_Temp(t1);
        break;
    }
    case t_TEMP:
    case t_CONST:
        break;
    default: assert(0 && "invalid subscript offset expr");
    }

    // Work out where the array will be stored, depending on its scope
    // This will be in data, in local or from a reference
    switch(sym_scope(p->sym)) {
    
    case t_scope_system: {
        label l = lblMap_getNamed(s->lbl, name);
        return i_Sys(i_Name(l), offset);
    }

    case t_scope_module:
        base = i_Name(ir_dpLoc(s->ir, name));
        return i_Mem(t_mem_dp, base, offset);
    
    // Array base will be local or by reference
    case t_scope_proc:
    case t_scope_func:
        switch(sym_type(p->sym)) {

        // MEM[CONST + (TEMP | CONST)]
        case t_sym_intArray: 
        case t_sym_chanArray: {
            
            base = i_Const(frm_arrayOffset(f, name));

            // If offset CONST we can load relative to sp
            switch(offset->type) {
            case t_CONST:
                return i_Mem(t_mem_spl, NULL, 
                        i_Const(base->u.CONST+offset->u.CONST));

            // Otherwise we must do an absoloute load
            case t_TEMP: {

                // t := &sp[base]
                temp t = frm_addNewTemp(f, t_tmp_local);
                list_add(stmts, i_Move(i_Temp(t), i_Mem(t_mem_spa, NULL, base)));
                
                // mem[t + off]
                return i_Mem(t_mem_abs, i_Temp(t), offset);
            }
            default: assert(0 && "Offset not of type CONST or TEMP");
            }
        }

        // MEM[TEMP + (TEMP|CONST)]
        case t_sym_intArrayRef:
            base = i_Temp(frm_addTemp(f, name, t_tmp_local));
            if(offset->type != t_TEMP) {
                temp t = frm_addNewTemp(f, t_tmp_local);
                list_add(stmts, i_Move(i_Temp(t), offset));
                offset = i_Temp(t);
            }
            return i_Mem(t_mem_abs, base, offset);
        default: assert(0 && "subscript not sym type array or ref");
        }
        break;
    default:
        assert(0 && "invalid scope");
    }
}

// Function call, returning a value element
static i_expr elem_FCall(structures s, frame f, a_elem p, list stmts) {
    label l = lblMap_getNamed(s->lbl, p->u.call.name->name);
    list args = list_New();
    argList(s, f, p->u.call.exprList, args, stmts);
    
    // Add this child call to the frame's list
    ir_addChildCall(s->ir, frm_name(f), lbl_name(l));
    
    return i_FCall(i_Name(l), args);
}

// Number
static i_expr elem_number(a_elem p) {
    return i_Const(p->u.number);
}

// Boolean
static i_expr elem_boolean(a_elem p) {
    return i_Const(p->u.boolean);
}

// Pass a reference of a string element
static i_expr elem_string(a_elem p) {
    return i_Mem(t_mem_cpa, i_Name(p->u.string.l), i_Const(0));
}

// ==================================================================
// Expression evaluation
// ==================================================================

// Evaluate two operands with a particular operator
int trl_evalOp(t_expr type, int a, int b) {
    switch(type) {
    case t_expr_plus:   return a + b;
    case t_expr_minus:  return a - b;
    case t_expr_mult:   return a * b;
    case t_expr_div:    return a / b;
    case t_expr_rem:    return a % b;
    case t_expr_or:     return a | b;
    case t_expr_and:    return a & b;
    case t_expr_xor:    return a ^ b;
    case t_expr_lshift: return a << b;
    case t_expr_rshift: return a >> b;
    case t_expr_eq:     return a == b ? TRUE : FALSE;
    case t_expr_ne:     return a != b ? TRUE : FALSE;
    case t_expr_ls:     return a < b  ? TRUE : FALSE;
    case t_expr_le:     return a <= b ? TRUE : FALSE;
    case t_expr_gr:     return a > b  ? TRUE : FALSE;
    case t_expr_ge:     return a >= b ? TRUE : FALSE;
    default: assert(0 && "Invalid a_expr type");
    }
}

// ==================================================================
// Helper functions
// ==================================================================

// PCall or FCall argument list
static void argList(structures s, frame f, a_exprList p, list args, list stmts) {
    if(p != NULL) {
        i_expr arg = expr(s, f, p->head, stmts);
        
        // Lift any arg not TEMP
        if(arg->type != t_TEMP) {
            temp t = frm_addNewTemp(f, t_tmp_local);
            list_add(stmts, i_Move(i_Temp(t), arg));
            arg = i_Temp(t);
        }

        list_add(args, arg);
        argList(s, f, p->tail, args, stmts);
    }
}

// Stmt seq
void stmt_seq(structures s, frame frm, a_stmtSeq p, list stmts) {
    if(p!=NULL) { 
        stmt(s, frm, p->head, stmts);
        stmt_seq(s, frm, p->tail, stmts);
    } 
}

// Stmt par
// --------
//    FORK .T0, .T1, .T2
//    FORKSET
//    FORKSET
//    FORKSET
//    FORKSYNC
//  .T1:
//      ...
//    JOIN
//  .T2:
//      ...
//    JOIN
//  .T3:
//      ...
//    JOIN
//  .exit:
void stmt_par(structures s, frame f, a_stmtPar p, list stmts) {

    a_stmtPar par;

    // Add temporaries it needs
    temp t1 = frm_addNewTemp(f, t_tmp_local);
    temp t2 = frm_addNewTemp(f, t_tmp_local);
    temp t3 = frm_addNewTemp(f, t_tmp_local);

    // Fork
    list threadLabels = list_New();
    list_add(stmts, i_Fork(i_Temp(t1), i_Temp(t2), i_Temp(t3), threadLabels));
    
    // Create the FORK_SET statements
    bool master = true;
    for(par=p; par!=NULL; par=par->tail) {
        
        // Create a thread label
        label l = lblMap_NewLabel(s->lbl);
        list_add(threadLabels, l);
        
        if(!master) 
            list_add(stmts, i_ForkSet(i_Temp(t1), i_Temp(t2), i_Temp(t3), l));
        master = false;
    }

    // ForkSync
    list_add(stmts, i_ForkSync(i_Temp(t1)));
    
    // Check number of threads
    if(list_size(threadLabels) > MAX_THREADS)
        err_report(t_error, -1, "insufficient threads");

    label exit = lblMap_NewLabel(s->lbl);

    iterator lblIt = it_begin(threadLabels);
    master = true;
    for(par=p; par!=NULL; par=par->tail) {

        // Add a thread label
        list_add(stmts, i_Label(it_next(lblIt)));

        // Add its statements and a concluding join
        stmt(s, f, par->head, stmts);
        list_add(stmts, i_Join(i_Temp(t1), master, exit));
        master = false;
    }

    // Exit
    list_add(stmts, i_Label(exit));
}

