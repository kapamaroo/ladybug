#ifndef _ERR_BUFF_H
#define _ERR_BUFF_H

#include <stdio.h> //FILE

//number of errors before abord parsing (0 for infinite)
#define ERR_NUM_LIMIT 0

#define MAX_ERROR_BUFFER 512
#define LOG_FILENAME "log"

extern int err_num;
extern FILE *log_file;
extern char str_err[MAX_ERROR_BUFFER];

//yyerror,yylineno are declared in bison.y
extern char *current_file;
extern int yylineno;
extern void yyerror(const char *msg);

void yywarning(char *msg);
void die(char *msg);

void init_err_buff();

#endif
