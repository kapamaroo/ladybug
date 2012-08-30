#ifndef _SUBPROGRAMS_H
#define _SUBPROGRAMS_H

#include "semantics.h"
#include "symbol_table.h"
#include "statements.h"

//performs basic actions/checks and creates a new subprogram/module
void subprogram_init(sem_t *sem_sub);

//closes a subprogram/module scope, cleans up the place, local symbol table, etc..
void subprogram_finit(sem_t *sem_sub, statement_t *body);

void configure_formal_parameters(param_list_t *list, func_t *func);

//postpone subprogram/module definition for later
void forward_subprogram_declaration(sem_t *subprogram);

//handlers for function/procedure declarations
sem_t *declare_function_header(char *id, param_list_t *list, data_t *return_type);
sem_t *declare_procedure_header(char *id, param_list_t *list);

#endif
