#ifndef _SUBPROGRAMS_H
#define _SUBPROGRAMS_H

#include "semantics.h"
#include "ir.h"
#include "bison.tab.h"

param_list_t *param_insert(param_list_t *new_list,pass_t mode,data_t *type);

void configure_formal_parameters(param_list_t *list,func_t *func);
void subprogram_init(sem_t *subprogram);
void subprogram_finit(sem_t *subprogram,ir_node_t *body);

void forward_subprogram_declaration(sem_t *subprogram);
sem_t *declare_function_header(char *id,param_list_t *list,data_t *return_type);
sem_t *declare_procedure_header(char *id,param_list_t *list);

#endif