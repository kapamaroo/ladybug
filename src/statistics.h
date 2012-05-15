#ifndef _STATISTICS_H_
#define  _STATISTICS_H_

#include "semantics.h"

#define MAX_BLOCK_SIZE 128

typedef struct stat_vars_t {
    unsigned int size; //num of statements (high level)
    unsigned int depth; //depth of nested loop, 0 for outermost loop

    //write and call statements have multiple read dependencies and no write dependencies
    var_list_t *write;
    var_list_t *read;
} stat_vars_t;

#endif
