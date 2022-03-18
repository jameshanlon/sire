#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "util.h"

#define RULE_LEN 40

//========================================================================
// Memory
//========================================================================

void *chkalloc(size_t size) {
    void *p = malloc(size);
    if(!p) {
        fprintf(stderr, "Error: insufficient memory\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

//========================================================================
// Strings
//========================================================================

string String(string s) {
    string p = chkalloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

string StringFmt(const string format, ...) {
    char buf[100];
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);
    return String(buf);
}

string StringCat(string a, string b) {
    char new[strlen(a)+strlen(b)+1];
    strcpy(new, a);
    strcat(new, b);
    return String(new);
}

string SubStr(string a, size_t from, size_t to) {
    char new[to-from];
    strncpy(new, a+from, to-from);
    new[to-from] = '\0';
    return String(new);
}

int streq(string a, string b) {
    return !strcmp(a, b);
}

void strDel(string s) {
    free(s);
}

string lowerCase(string s) {
    char l[strlen(s)+1];
    unsigned int i, j = 'a' - 'A';
    for(i=0; i<strlen(s); i++)
        l[i] = s[i] + j;
    l[strlen(s)] = '\0';
    return String(l);
}

//========================================================================
// Printing
//========================================================================

static string ruleStr(int len) {
    char s[len+1];
    int i;
    for(i=0; i<len; i++)
        s[i] = '=';
    s[len] = '\0';
    return String((string) &s);
}

void printTitleRule(FILE *out, string title) {
    fprintf(out, "%s %s\n", title, ruleStr(RULE_LEN-strlen(title)-1));
}

void printRule(FILE *out) {
    fprintf(out, "%s\n", ruleStr(RULE_LEN));
}

void printLn(FILE *out) {
    fprintf(out, "\n");
}

