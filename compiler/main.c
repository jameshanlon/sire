#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "util.h"
#include "list.h"
#include "error.h"

#include "statistics.h"
#include "symbol.h"
#include "ast.h"
#include "astprinter.h"
#include "structures.h"
#include "sem.h"
#include "translate.h"
#include "irt.h"
#include "irtprinter.h"
#include "ir.h"
#include "block.h"
#include "regalloc.h"
#include "codegen.h"

#define ASM  "xas"
#define COMP "xcc"
#define LINK "xcc"

typedef enum {
    tgt_SIM,
    tgt_XC1,
    tgt_XMP64
} t_target;

char targetStr[][12] = {
    "XC-1",
    "XC-1",
    "XK-XMP-64"
};

int numCores[] = {
    1,
    4,
    64
};

// Global options and variables
bool       displayAst;
bool       displayIrt;
bool       compileOnly;
bool       assembleOnly;
bool       verbose;
bool       stats;
bool       outputAsm;
FILE      *in;
FILE      *asmOut;
FILE      *jumpTabOut;
FILE      *cpOut;
string     inFile;
string     asmFile;
string     jumpTabFile;
string     cpFile;
string     elfFile;
string     xeFile;
t_target   target;
a_module   astRoot;
structures s;

// Print some usage info
void printHelp(void) {
    printf("Usage: cmp [options] <source-file>\n");
    printf("Options:\n");
    printf("  -h          Display this help message\n");
    printf("  -v          Verbose\n");
    printf("  -ast        Display AST and quit\n");
    printf("  -ir         Display IR and quit\n");
//    printf("  -t=<target> Specify the target device\n"); 
//    printf("  -s          Display compilation statistics\n");
//    printf("  -S          Compile, do not assemble\n");
//    printf("  -c          Compile and assemble, do not link\n");
    printf("  -o <file>   Output file\n");
}

// Set a target
void setTarget(char *arg) {
    char tgt[10];
    int i=0;
    arg += 3;
    while(arg[i] != '\0') { tgt[i] = arg[i]; i++; }
    if(streq(tgt, "sim")) target = tgt_SIM;
    else if(streq(tgt, "xc1")) target = tgt_XC1;
    else if(streq(tgt, "xmp64")) target = tgt_XMP64;
    else err_fatal("invalid target");
}

// Parse command line options and input files
int parseOptions(int argc, char **argv) {
   
    // Default options
    displayAst   = false;
    displayIrt   = false;
    compileOnly  = false;
    assembleOnly = false;
    verbose      = false;
    stats        = false;
    asmFile      = "program.S";
    jumpTabFile  = "jumpTable.S";
    cpFile       = "cp.S";
    elfFile      = "out.o";
    xeFile       = "out.xe";
    target       = tgt_XC1;

    // Get options
    while((argc > 1) && (argv[1][0] == '-')) {
        switch(argv[1][1]) {
        case 'h': printHelp();              return FAIL;
        case 'v': verbose = true;           break;
        case 'a': displayAst = true;        break;
        case 'i': displayIrt = true;        break;
        case 't': setTarget(argv[1]);       break;
        case 's': stats = true;             break;
        case 'S': compileOnly = true;       break;
        case 'c': assembleOnly = true;      break;
        case 'o': xeFile = String(argv[1]); break;
        default:
            err_fatal("invalid option %s", argv[1]);
            return FAIL;
        }
        ++argv;
        ++argc;
    }
    
    // Print help if no input file
    if(argc == 1) {
        printHelp();
        return FAIL;
    }
    
    inFile = argv[1];
    return SUCCESS;
}

// Lexical analysis stage
int stage_lex() {
    
    extern FILE *yyin;
    extern int yyparse(a_module *);
    
    if(verbose) printf("Performing lexical analysis\n");

    if(!streq(inFile, "stdin"))
        in = fopen(inFile, "r");
    else in = stdin;
    
    if(in == NULL) {
        err_fatal("opening input file: %s", inFile);
        return FAIL;
    }

    err_init(inFile);
    yyin = in;
    
    if(yyparse((a_module *) &astRoot) == 0) {
        if(!err_anyErrors()) {
            fclose(yyin);
            return SUCCESS;
        }
        else {
            err_summary();
            return FAIL;
        } 
    }
    else {
        err_summary();
        return FAIL;
    }
}

// Semantic analysis stage
int stage_sem() {

    if(verbose) printf("Performing semantic analysis\n");

    analyse(s, astRoot);
    
    if(err_anyErrors()) {
        err_summary();
        return FAIL;
    }

    return SUCCESS;
}

// Intermediate representation generation stage
int stage_irt() {
    
    if(verbose) printf("Translating IR\n");
    
    translate(s);
    return SUCCESS;
}

// Basic block sequencing stage
int stage_seq() {
    
    if(verbose) printf("Sequencing basic blocks\n");
    
    basicBlocks(s);
    return SUCCESS;
}

// Register allocation stage
int stage_reg() {
    
    if(verbose) printf("Allocating registers\n");
    
    allocRegs(s); 
    return SUCCESS;
}

// Code generation stage
int stage_gen() {
    
    if(verbose) printf("Generating %s and %s\n", asmFile, cpFile);
   
    asmOut = fopen(asmFile, "w");
    if(asmOut == NULL) { 
        err_fatal("opening asm file %s\n", asmFile); 
        return FAIL; 
    }
    
    jumpTabOut = fopen(jumpTabFile, "w");
    if(jumpTabOut == NULL) { 
        err_fatal("opening asm file %s\n", jumpTabFile); 
        return FAIL; 
    }
    
    cpOut = fopen(cpFile, "w");
    if(cpOut == NULL) { 
        err_fatal("opening cp file %s\n", cpFile); 
        return FAIL; 
    }

    // Generate the assembly code
    gen_program(asmOut, jumpTabOut, cpOut, s);
   
    fclose(asmOut);
    fclose(jumpTabOut);
    fclose(cpOut);

    return SUCCESS;
}

// Assemble the outFile into a binary
int stage_asm() {
    
    if(verbose)
        printf("Assembly\n");
    
    int e;

    if(system(NULL)) {
        string s = StringFmt("xas %s -o %s", asmFile, elfFile);
        printf("%s\n", s); 
        e = system(s);
    } else {
        err_fatal("invoking assembler\n");
        return FAIL;
    }
    
    return SUCCESS;
}

// Link binary into a multi-core XE file
/*int stage_bin() {

    if(verbose)
        printf("Creating XE\n");

    //string wrapper = "a.xc";
    //string wrapperObj = "a.o";
    string kFuncs = "kernel/functions.xc";
    string kFuncsObj = "functions.o";
    string kernel = "kernel/kernel.s";
    string kernelObj = "kernel.o";

    // Open the wrapper file
    FILE *p = fopen(wrapper, "w");
    if(p == NULL) {
        err_fatal("failed to write wrapper file: %s", wrapper);
        return FAIL;
    }

    // Write the wrapper file
    fprintf(p, "#include <platform.h>\n");
    fprintf(p, "extern void %s();\n", INIT_FUNC);
    fprintf(p, "int main(void) {\n");
    if(numCores[target] > 1) {
        fprintf(p, "\tpar{\n");
        fprintf(p, "\t\ton stdcore[0] : %s();\n", INIT_FUNC);
        fprintf(p, "\t\tpar(int i=1; i<%d; i++)\n", numCores[target]);
        fprintf(p, "\t\t\ton stdcore[i] : %s();\n", INIT_FUNC);
    }
    else {
        fprintf(p, "\t%s();\n", INIT_FUNC);
    }
    fprintf(p, "\t}\n\treturn 0;\n}\n");
    fclose(p);

    // Compile the wrapper and link into XE
    if(system(NULL)) {

        // Compile & assemble but don't link wrapper
        //string s1 = StringFmt("xcc -O2 -target=%s -c %s -o %s", 
        //        targetStr[target], wrapper, wrapperObj);
        
        // Compile & assemble but don't link the kernel functions
        string s2 = StringFmt("xcc -O2 -target=%s -c %s -o %s",
                targetStr[target], kFuncs, kFuncsObj);

        string s3 = StringFmt("xas %s -o %s", "kernel/initMaster.s", "initMaster.o");

        string s4 = StringFmt("xas %s -o %s", "kernel/initSlave.s", "initSlave.o");

        // Assemble but don't link the kernel
        string s5 = StringFmt("xas %s -o %s", kernel, kernelObj);

        // Compile and link wrapper, kernel, k functions and code
        string args = StringFmt("-target=%s -nostdlib -Xmapper --nochaninit", targetStr[target]); 

        string s6 = StringFmt("xcc %s -Xmapper -first initMaster.o initMaster.o %s %s %s -o master.xe", 
                args, kernelObj, kFuncsObj, elfFile);

        string s7 = StringFmt("xcc %s -Xmapper -first initSlave.o initSlave.o %s %s -o slave.xe", 
                args, kernelObj, kFuncsObj);
        
        string s8 = StringFmt("xobjdump --split slave.xe");

        // Execute the commands
        //printf("%s\n", s1); system(s1); 
        printf("%s\n", s2); system(s2); 
        printf("%s\n", s3); system(s3);
        printf("%s\n", s4); system(s4);
        printf("%s\n", s5); system(s5);
        printf("%s\n", s6); system(s6);
        printf("%s\n", s7); system(s7);
        printf("%s\n", s8); system(s8);

        // Replace the slave images into the master image
        for(int i=1; i<numCores[target]; i++) {
            string s = StringFmt("xobjdump master.xe -a %d,%d,image_n0c0.elf",
                    i/4, i%4);
            printf("%s\n", s); system(s);
        }

    } else {
        err_fatal("creating XE file %s", xeFile);
        return FAIL;
    }

    // delete: a.o out.o kernel.o a.xc out.s

    return SUCCESS;
}*/

// Main method parses command line options, invokes top-level compiler phases,
// and then the assembler
int main(int argc, char **argv) {
    
    // Parse options
    if(parseOptions(argc, argv))
        return FAIL;

    // Initialise compiler objects
    s = structs_New();
    stats_init();
   
    // Front end
    if(stage_lex()) return FAIL;
    if(displayAst) {
        p_module(stdout, astRoot);
        return SUCCESS;
    }

    if(stage_sem()) return FAIL;
    if(stage_irt()) return FAIL;

    // Middle
    //ir_display(s->ir, stdout);
    if(stage_seq()) return FAIL;
    if(stage_reg()) return FAIL;
    if(displayIrt) {
        ir_display(s->ir, stdout);
        return SUCCESS;
    }
    
    // Back end
    if(stage_gen()) return FAIL;
    if(compileOnly) return SUCCESS;

    // Assemble into ELF
    //if(stage_asm()) return FAIL;
    //if(assembleOnly) return SUCCESS;

    // Create XE
    //if(stage_bin()) return FAIL;

    // Display statistics info
    if(stats) stats_dump(stdout);

    return SUCCESS;
}

