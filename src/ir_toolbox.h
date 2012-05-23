#ifndef _IR_TOOLBOX_H
#define _IR_TOOLBOX_H

#include "semantics.h"
#include "ir.h"

#define MAX_LABEL_SIZE 64
#define DEFAULT_LABEL_PREFIX "label"

var_t *variable_from_comp_datatype_element(var_t *var);

expr_t *make_enum_subset_bound_checks(var_t *v,expr_t *l);
expr_t *make_ASCII_bound_checks(var_t *v,expr_t *l);
ir_node_t *prepare_stack_and_call(func_t *subprogram, expr_list_t *list);

ir_node_t *expr_tree_to_ir_cond(expr_t *ltree);
ir_node_t *expr_tree_to_ir_tree(expr_t *ltree);
ir_node_t *calculate_lvalue(var_t *v);

op_t op_invert_cond(op_t op);
#endif
