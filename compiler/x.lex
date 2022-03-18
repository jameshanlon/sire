%{
#include <string.h>
#include <math.h>
#include "error.h"
#include "symbol.h"
#include "ast.h"
#include "x.tab.h"

void adj(void) {
    tp = cp;
    cp += yyleng;
}
%}

/*%option debug*/
%option bison-bridge

/* Character sets */
WHITE      [ \t]+
NEWLINE    [\n\r]
DIGIT      [0-9]
HEXDIGIT   [0-9A-Fa-f]
BINDIGIT   [01]

/* Terminals */
ID         [A-Za-z][a-zA-Z0-9_]*

DECLITERAL {DIGIT}+
HEXLITERAL 0[xX]{HEXDIGIT}+
BINLITERAL 0[bB]{BINDIGIT}+

CHARCONST  \'([^\']|"\\n"|"\\t")\'
STRING     \"[^\"]*\"

COMMENT    %.*$

%%

{WHITE}   { adj();                   }
{COMMENT} { adj();                   }
{NEWLINE} { adj(); err_newLine();    }

"aliases" { adj(); return ALIASES;   }
"chanend" { adj(); return CHANEND;   }
"connect" { adj(); return CONNECT;   }
"const"   { adj(); return CONST;     }
"int"     { adj(); return INT;       }
"do"      { adj(); return DO;        }
"to"      { adj(); return TO;        }
"else"    { adj(); return ELSE;      }
"false"   { adj(); return FALSE;     }
"for"     { adj(); return FOR;       }
"func"    { adj(); return FUNC;      }
"if"      { adj(); return IF;        }
"is"      { adj(); return IS;        }
"on"      { adj(); return ON;        }
"port"    { adj(); return PORT;      }
"proc"    { adj(); return PROC;      }
"return"  { adj(); return RETURN;    }
"skip"    { adj(); return SKP;       }
"then"    { adj(); return THEN;      }
"true"    { adj(); return TRUE;      }
"var"     { adj(); return VAR;       }
"while"   { adj(); return WHILE;     }
".."      { adj(); return DOTS;      }

"["       { adj(); return LBRACKET;  }
"]"       { adj(); return RBRACKET;  }
"("       { adj(); return LPAREN;    } 
")"       { adj(); return RPAREN;    }
"{"       { adj(); return START;     }
"}"       { adj(); return END;       }
";"       { adj(); return SEMICOLON; }
"|"       { adj(); return BAR;       }
","       { adj(); return COMMA;     }
"+"       { adj(); return PLUS;      }
"-"       { adj(); return MINUS;     }
"*"       { adj(); return MULT;      }
"/"       { adj(); return DIV;       }
"rem"     { adj(); return REM;       }
"or"      { adj(); return OR;        }
"and"     { adj(); return AND;       }
"xor"     { adj(); return XOR;       }
"$"       { adj(); return MASTER;    }
":="      { adj(); return ASS;       }
"?"       { adj(); return INPUT;     }
"!"       { adj(); return OUTPUT;    }
"<<"      { adj(); return LSHIFT;    }
">>"      { adj(); return RSHIFT;    }
"="       { adj(); return EQ;        }
"<"       { adj(); return LS;        }
"<="      { adj(); return LE;        }
">"       { adj(); return GR;        }
">="      { adj(); return GE;        }
"~"       { adj(); return NOT;       }
"~="      { adj(); return NE;        }
":"       { adj(); return COLON;     }

{CHARCONST} { 
    adj();
    if(streq(yytext, "'\\n'")) {
        yylval->intval = (char) '\n';
    }
    else
        yylval->intval = (char) yytext[1];
    return NUMBER;
}

{STRING} {  
    adj();
    
    char *u = yytext;
    char v[255];
    int i, j = 0;

    // Convert escape characters
    for(i=1; i<strlen(u)-1; i++) {
        /*if(u[i] == '\\') {
            switch(u[++i]) {
            case 'n': v[j] = '\n'; break;
            case 't': v[j] = '\t'; break;
            default: assert(0 && "unrecognised escape character");
            }
        }
        else*/ v[j] = u[i];
        j++;
    }
    v[j] = '\0';

    yylval->strval = String(v);
    //printf("got string '%s'\n", yylval->strval);
    return STRING;
}

{ID} { 
    adj(); 
    yylval->strval = (string) strdup(yytext);
    return ID; 
}

{DECLITERAL} { 
    adj();
    yylval->intval = atoi(yytext); 
    return NUMBER; 
}

{HEXLITERAL} { 
    adj();
    yylval->uintval = (int) strtoul(yytext+2, NULL, 16);
    //printf("get hex number %s as %x\n", yytext+2, yylval->uintval);
    return NUMBER; 
}

{BINLITERAL} { 
    adj();
    yylval->uintval = (int) strtoul(yytext+2, NULL, 2);
    //printf("get bin number %s as %b\n", yytext+2, yylval->uintval);
    return NUMBER; 
}

. { err_report(t_error, tp, "unrecognised characrter"); }

%%
