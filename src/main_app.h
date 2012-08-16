#ifndef _MAIN_APP_H
#define _MAIN_APP_H

#include "build_flags.h"

#if LEX_MAIN == 0

void help();

extern int enable_opt_loop_unroll_classic;
extern int enable_opt_loop_unroll_symbolic;
extern int enable_opt_loop_simplify;

extern char *src_input;

void parse_args(int argc, char *argv[]);

#endif

#endif
