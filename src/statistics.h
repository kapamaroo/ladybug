#ifndef _STATISTICS_H_
#define  _STATISTICS_H_

#include "semantics.h"

/*** Loop statistics ***/

#define MAX_LOOP_SIZE 128

typedef struct loop_stats_t {
    unsigned int size; //num of statements, not including init code, condition check and step statement
    unsigned int depth; //depth of nested loop, 0 for outermost loop
    var_t *dependencies[MAX_LOOP_SIZE];
} loop_stats_t;

#endif
