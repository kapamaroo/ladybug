#ifndef _SCOPE_H
#define _SCOPE_H

#include "semantics.h"

#define MAX_SCOPE 10

extern int sm_scope;
extern scope_t scope_stack[MAX_SCOPE+1];
extern func_t *main_program;
//scope 0 is the scope of the main program and if the
//symbol table is an array, it is the first element
//scope_stack[n] points to the first element of n-th scope

typedef struct with_stmt_scope_t {
    data_t *type; //record type of with statement
    //only if conflicts==0 it is allowed to close a with_scope, else conflicts--
    int conflicts; //(non negative) remember how many with_scopes failed to open due to element name conflicts
    struct with_stmt_scope_t *prev;
    struct with_stmt_scope_t *next;
} with_stmt_scope_t;

extern with_stmt_scope_t *root_scope_with;
//extern with_stmt_scope_t *tail_scope_with;

void init_scope();

void start_new_scope(func_t *scope_owner);
void close_current_scope();
void sm_clean_current_scope();

scope_t *get_current_scope();
func_t *get_current_scope_owner();

void start_new_with_statement_scope(var_t *var);
void close_last_opened_with_statement_scope();

#endif
