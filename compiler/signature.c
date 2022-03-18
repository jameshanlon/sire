#include <stdlib.h>
#include "table.h"
#include "signature.h"
#include "statistics.h"

// Procedure signature data type
struct signature_ {
    string name;
    int numFormals;
    t_formal *args;
    signature prev;
};

// Signature table data structure
struct sigTable_ {
    table tab;
    signature top;
};

static void printSig(signature, FILE *);

// Constructor
sigTable sigTab_New(void) {
    sigTable t = (sigTable) chkalloc(sizeof(*t));
    t->tab = tab_New();
    t->top = NULL;
    return t;
}

// Construct a procedure signature
static signature Signature(a_procDecl p, signature prev) {

    signature sig = (signature) chkalloc(sizeof(*sig)); 
    sig->name = String(p->name->name);
    sig->numFormals = 0;
    sig->prev = prev;

    a_formals f;
    a_paramDeclSeq s;

    // Count the number of formal args
    for(f=p->formals; f!=NULL; f=f->next) {
        for(s=f->params; s!=NULL; s=s->next)
            sig->numFormals++;
    }
       
    // Initialse an array with their types
    sig->args = (t_formal *) chkalloc(sizeof(t_formal) * sig->numFormals);
    int i = 0;
    for(f=p->formals; f!=NULL; f=f->next) {
        for(s=f->params; s!=NULL; s=s->next)
            sig->args[i++] = f->type;
    }

    //printf("added signature: "); printSig(sig, stdout);
    return sig;
}

// Insert a signature in the table
signature sigTab_insert(sigTable t, a_procDecl p) {
    t->top = Signature(p, t->top);
    tab_insert(t->tab, t->top->name, t->top);
    stat_numProcedures++;
    return t->top;
}

// Return the corresponding formal type of a symbol from a procedure 
// argument expression: (convert t_symbol -> t_formal)
static t_formal getFmlType(symbol s) {
    //printf("getFmlType: %s\n", sym_name(s));
    switch(sym_type(s)) {
    case t_sym_const:        return t_formal_int;
    case t_sym_chan:         return t_formal_chanend;
    case t_sym_chanend:      return t_formal_chanend;
    case t_sym_port:         return t_formal_port;
    case t_sym_int:          return t_formal_int;
    case t_sym_intArray:     return t_formal_intArray;
    case t_sym_intAlias:     return t_formal_intArray;
    case t_sym_intArrayRef:  return t_formal_intArray;
    default: assert(0 && "Invalid symbol type");
    }
    return -1;
}

// Get the type of a formal argument expression
static t_formal getType(symTable t, a_expr expr) {

    //printf("getting type for %d\n", expr->type);
    
    // Inspect the single element
    if(expr->type == t_expr_none) {
        a_elem elem = expr->u.monadic.elem;
        //printf("\t elem type  %d\n", elem->type);
        switch(elem->type) {
        case t_elem_expr:    return getType(t, elem->u.expr);
        case t_elem_number:  return t_formal_int;
        case t_elem_boolean: return t_formal_int;
        case t_elem_string:  return t_formal_intArray;
        case t_elem_fCall:   return t_formal_int;
        case t_elem_sub:     return t_formal_int;
        case t_elem_name: 
            return getFmlType(symTab_lookup(t, elem->u.name->name));
        default: assert(0 && "Invalid expr type");
        }
    }
    
    // OR: Any monadic or diadic operations will supply a value
    return t_formal_int;
}

// Lookup a signature by a string key
signature sigTab_lookup(sigTable sig, string key) {
    return tab_lookup(sig->tab, key);
}

// Check the arguments of a procedure call match those of the signature
// NOTE: with modules, lookup will have to be performed with parent name
bool sigTab_checkArgs(sigTable sig, symTable sym, string key, a_exprList exprList) {
    
    signature s = tab_lookup(sig->tab, key);
    //printf("checkArgs for %s:\n", key);
   
    // Check if a signature exists
    if(s == NULL)
        return false; 
    
    // Check each argument
    int i=0;
    for(; exprList!=NULL; exprList=exprList->tail) {
        t_formal type = getType(sym, exprList->head);
        
        //printf("\tsig: %s, call: %s\n", 
        //        a_formalTypeStr(s->args[i]), 
        //        a_formalTypeStr(getType(sym, exprList->head)));
        
        if(type != s->args[i]) {

            // Check for a valid name param to val formal match
            //if(s->args[i] == t_formal_val && type == t_formal_var)
            //    return true;
            
            // Otherwise arg type does not match formal
            return false;
       }
       i++;
    }

    // Check if there were the correct number of arguments
    if(i != s->numFormals)
        return false;

    return true;
}

int sigTab_numArgs(sigTable t, string key) {
    signature s = tab_lookup(t->tab, key);
    assert(s != NULL && "Signature NULL");
    return s->numFormals;
}

t_formal sig_getFmlType(signature s, int arg) {
    return s->args[arg];
}

static void printSig(signature s, FILE *out) {
    int i;
    fprintf(out, "%s [%d] (", s->name, s->numFormals);
    for(i=0; i<s->numFormals; i++) {
        fprintf(out, "%s%s ", 
                a_formalTypeStr(s->args[i]), 
                i+1==s->numFormals? "" : ", ");
    }
    fprintf(out, ")\n");
}

// Dump all of the symbols from the table
void sigTab_dump(sigTable t, FILE *out) {
    signature s = t->top;
    printf("Procedure signatures:\n");
    for(; s!=NULL; s=s->prev) {
        fprintf(out, "\t");
        printSig(s, out);
    }
}

