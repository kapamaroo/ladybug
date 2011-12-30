#ifndef _IR_TOOLBOX_H
#define _IR_TOOLBOX_H

#include "semantics.h"
#include "ir.h"

#define MAX_LABEL_SIZE 64
#define DEFAULT_LABEL_PREFIX "label"

expr_t *make_enum_subset_bound_checks(var_t *v,expr_t *l);
expr_t *make_ASCII_bound_checks(var_t *v,expr_t *l);
ir_node_t *prepare_stack_and_call(func_t *subprogram, expr_list_t *list);

ir_node_t *expr_tree_to_ir_tree(expr_t *ltree);
ir_node_t *calculate_lvalue(var_t *v);
var_t *new_normal_variable_from_guarded(var_t *guarded);

int check_assign_similar_comp_datatypes(data_t* vd,data_t *ld);

#endif
