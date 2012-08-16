#ifndef _OPT_FOR_STMT_H
#define _OPT_FOR_STMT_H

#include "statements.h"

enum opt_loop_type {
    OPT_UNROLL_CLASSIC,
    OPT_UNROLL_SYMBOLIC,
    OPT_LOOP_SIMPLIFY,
};

void init_opt_for();

//optimizations' wrapper, use this instead of direct optimizing
void optimize_loops(enum opt_loop_type type);

#endif
