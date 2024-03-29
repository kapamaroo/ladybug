#include <stdio.h>
#include <stdlib.h>

#include "build_flags.h"
#include "err_buff.h"

int err_num;
FILE *log_file;
char str_err[MAX_ERROR_BUFFER];

void init_err_buff() {
    err_num = 0;

#if LOG_TO_FILE == 1
    log_file = fopen(LOG_FILENAME,"w");
#endif
}

void yywarning(char *msg) {
    printf("%s:%d:warning: %s\n",current_file,yylineno,msg);
#if LOG_TO_FILE == 1
    fprintf(log_file,"%s:%d:warning: %s\n",current_file,yylineno,msg);
#endif
}

void die(char *msg) {
    printf("%s\n",msg);
#if LOG_TO_FILE == 1
    fprintf(log_file,"%s\n",msg);
#endif
    exit(EXIT_FAILURE);
}
