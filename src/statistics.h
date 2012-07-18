#ifndef _STATISTICS_H_
#define  _STATISTICS_H_

#include "semantics.h"

#define MAX_BLOCK_SIZE 128

struct statement_t;

typedef struct stat_vars_t {
    unsigned int size; //num of statements (high level)
    unsigned int depth; //depth of nested loop, 0 for outermost loop

    //write and call statements have multiple read dependencies and no write dependencies
    var_list_t *write;
    var_list_t *read;
} stat_vars_t;

enum dependence_type {
    DEP_RAR,  //read after read
    DEP_RAW,  //read after write
    DEP_WAR,  //write after read
    DEP_WAW   //write after write
};

typedef struct dep_t {
    int index;  //index in pool
    enum dependence_type is;
    struct statement_t *from;
    struct statement_t *to;
    info_comp_t *conflict_info_from;
    info_comp_t *conflict_info_to;
    var_t *var_from;
    var_t *var_to;
} dep_t;

typedef struct dep_vector_t {
    var_t *guard;
    int size;  //size of pool
    int next_free_spot;  //existing elements in pool
    dep_t *pool;
} dep_vector_t;

#endif
