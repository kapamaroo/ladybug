#ifndef _STATISTICS_H_
#define  _STATISTICS_H_

#include "semantics.h"

//arbitrary upper limit of dynamically allocated arrays
#define MAX_BLOCK_SIZE 128

//avoid '#include' of statements.h
//we only need the struct
struct statement_t;

//I/O variable container for statements
//each statements knows which variables reads/writes
typedef struct stat_vars_t {
    //reminder: write and call statements have multiple
    //          read dependencies and no write dependencies

    var_list_t *write;
    var_list_t *read;
} stat_vars_t;

//types of statement dependencies
enum dependence_type {
    DEP_RAR,  //read after read
    DEP_RAW,  //read after write
    DEP_WAR,  //write after read
    DEP_WAW   //write after write
};

//representation of a single dependence between two statements
typedef struct dep_t {
    int index;  //index in pool (array implementation specific)
    enum dependence_type is;
    struct statement_t *from;
    struct statement_t *to;

    //if dep-variable points to an array element,
    //keep info for the array declaration
    info_comp_t *conflict_info_from;
    info_comp_t *conflict_info_to;

    //use for default dep cases
    //dep-variable's type is a standard/scalar type
    var_t *var_from;
    var_t *var_to;
} dep_t;

//representation of all dependencies inside a block of statements
typedef struct dep_vector_t {
    var_t *guard;        //use for analysis of for statements, loop var-itarator
                         //needs special treatment
                         //else set to NULL

    //array implementation of block dependencies
    int size;            //size of pool
    int next_free_spot;  //existing elements in pool
    dep_t *pool;         //actual pointer
} dep_vector_t;

#endif
