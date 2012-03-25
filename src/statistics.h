#ifndef _STATISTICS_H_
#define  _STATISTICS_H_

#include "semantics.h"

#define MAX_BLOCK_SIZE 128

typedef struct common_stats_t {
    //write and call statements have multiple read dependencies and no write dependencies
    var_list_t *write;
    var_list_t *read;
} common_stats_t;

typedef struct block_stats_t {
    unsigned int size; //num of statements (high level)
    unsigned int depth; //depth of nested loop, 0 for outermost loop
    common_stats_t *dependencies[MAX_BLOCK_SIZE];
} block_stats_t;

#endif
