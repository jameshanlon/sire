#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>
#include "util.h"

typedef enum { t_error, t_warning } err_type;

extern int tp; // token position
extern int cp; // character postion

void err_newLine(void);
void err_report(err_type, int, string, ...);
void err_fatal(string, ...);
void err_init(string);
bool err_anyErrors(void);
void err_summary(void);

#endif
