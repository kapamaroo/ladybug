#ifndef _STATISTICS_H_
#define  _STATISTICS_H_

#include "semantics.h"

/*** Loop statistics ***/

#define MAX_LOOP_SIZE 128

typedef struct loop_stats_t {
    unsigned int size; //num of statements, not including init code, condition check and step statement
    unsigned int depth; //depth of nested loop, 0 for outermost loop
    var_list_t *dependencies[MAX_LOOP_SIZE];
} loop_stats_t;

/*** Common statistics (assignment,read,write,call) ***/

typedef struct common_stats_t {
    //write and call statements have multiple read dependencies and no write dependencies
    var_list_t *write;
    var_list_t *read;
} common_stats_t;

#endif
