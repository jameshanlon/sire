#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define true    1
#define false   0
#define FAIL    1
#define SUCCESS 0

typedef char *string;
typedef char  bool;

// Memory
void    *chkalloc(size_t);

// String manipluation
string   String(string);
string   StringFmt(const string format, ...);
string   StringCat(string, string);
string   SubStr(string a, size_t from, size_t to);
int      streq(string, string);
void     strDel(string);
string   lowerCase(string s);

// Printing
void     printTitleRule(FILE *, string);
void     printRule(FILE *);
void     printLn(FILE *);

#endif
