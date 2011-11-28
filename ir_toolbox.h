#ifndef _IR_TOOLBOX_H
#define _IR_TOOLBOX_H

#include "semantics.h"
#include "ir.h"

#define MAX_LABEL_SIZE 64
#define DEFAULT_LABEL_PREFIX "autolabel"

char *new_label_literal(char *label);
char *new_label_unique(char *prefix);
char *new_label_true(char *branch_label);
char *new_label_false(char *branch_label);
char *new_label_loop_to_cond(char *branch_label);
char *new_label_exit_branch(char *branch_label);
char *new_label_subprogram(char *sub_name);

expr_t *make_enum_subset_bound_checks(var_t *v,expr_t *l);
expr_t *make_ASCII_bound_checks(var_t *v,expr_t *l);
ir_node_t *prepare_stack_for_call(func_t *subprogram, expr_list_t *list);

ir_node_t *expr_tree_to_ir_tree(expr_t *ltree);
ir_node_t *calculate_lvalue(var_t *v);
var_t *new_normal_variable_from_guarded(var_t *guarded);

ir_node_t *create_bitmap(expr_t *expr_set);
ir_node_t *create_basic_bitmap(var_t *factory,expr_t *expr_set);
ir_node_t *bitmap_generator(var_t *factory,expr_t *expr_set);

ir_node_t *make_bitmap_inop_check(expr_t *expr_inop);
ir_node_t *make_dynamic_inop_check(expr_t *expr_inop);

int check_assign_similar_comp_datatypes(data_t* vd,data_t *ld);

#endif
