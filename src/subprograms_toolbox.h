#ifndef _SUBPROGRAMS_TOOLBOX_H_
#define _SUBPROGRAMS_TOOLBOX_H_

#include "semantics.h"
#include "symbol_table.h"

sem_t *reference_to_forwarded_function(char *id);
param_list_t *param_insert(param_list_t *new_list, pass_t mode, data_t *type);
func_t *find_subprogram(char *id);

#endif
