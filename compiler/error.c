#include <stdlib.h>
#include <stdio.h>
#include "error.h"

typedef struct intList_ *intList;

struct intList_ {
    int i;
    intList next;
};

static string  filename;
static int     lineNum;
static intList linePos;
static int     numErrors;
static int     numWarnings;
int            tp;
int            cp;

// Add a new element to the intList
static intList Intlist(int i, intList rest) {
    intList l = chkalloc(sizeof(*l));
    l->i = i;
    l->next = rest;
    return l;
}

// Initialise error state and open the file
void err_init(string filename_) {
    numErrors = 0;
    numWarnings = 0;
    filename = filename_;
    lineNum = 1;
    linePos = Intlist(0, NULL);
    tp = 0;
}

// Called at each newline in input
void err_newLine(void) {
    lineNum++;
    linePos = Intlist(tp, linePos);
}

// Report an error
void err_report(err_type type, int tp, string format, ...) {
    intList lines = linePos;
    int num = lineNum;
    va_list ap;

    while(lines && lines->i >= tp) {
        lines = lines->next;
        num--;
    }

    // File and position
    if(filename != NULL)
        fprintf(stderr, "%s:", filename);
    if(lines != NULL)
        fprintf(stderr, "%d.%d: ", num, tp-lines->i);
    
    // Error type
    switch(type) {
    case t_warning:
        fprintf(stderr, "warning: ");
        numWarnings++;
        break;
    case t_error:
        fprintf(stderr, "error: ");
        numErrors++;
        break;
    default:
        assert(0 && "invalid error type");
    }

    // Error description
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

void err_fatal(string format, ...) {
    va_list ap;
    fprintf(stderr, "Error: ");
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

bool err_anyErrors(void) {
    return numErrors > 0;
}

// Summarise error output
void err_summary(void) {
    if(numErrors > 0 || numWarnings > 0)
        fprintf(stderr, "%d errors, %d warnings\n", numErrors, numWarnings);
}
