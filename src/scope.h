#ifndef _SCOPE_H
#define _SCOPE_H

#include "semantics.h"

#define MAX_SCOPE 10

extern func_t *main_program;
extern func_t internal_scope;

typedef struct with_stmt_scope_t {
    data_t *type;  //record type of with statement

    int conflicts;  //(non negative) counter of how many with_scopes failed to
                    //open due to element name conflicts
                    //only if conflicts==0 it is allowed to close a with_scope
                    //else conflicts--

    struct with_stmt_scope_t *prev;
    struct with_stmt_scope_t *next;
} with_stmt_scope_t;

extern with_stmt_scope_t *root_scope_with;

void init_scope();

void start_new_scope(func_t *scope_owner);
void close_scope(func_t *scope_owner);
void sm_clean_current_scope(func_t *scope_owner);

func_t *get_current_scope_owner();

void start_new_with_statement_scope(var_t *var);
void close_last_opened_with_statement_scope();

#endif
