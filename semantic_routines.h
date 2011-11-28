#ifndef _SEMANTIC_ROUTINES_H
#define _SEMANTIC_ROUTINES_H

#include "semantics.h"
#include "ir.h"
#include "bison.tab.h"

//here comes the semantic routines
void init_semantics();

void make_type_definition(char *id, data_t *type);

dim_t *make_dim_bound_from_id(char *id);
dim_t *make_dim_bounds(expr_t *l1,expr_t *l2);
var_t *refference_to_array_element(var_t *v, expr_list_t *list);

int valid_expr_list_for_array_reference(data_t *data,expr_list_t *list);
expr_t *make_array_refference(expr_list_t *list,data_t *data);
expr_t *make_array_bound_check(expr_list_t *list,data_t *data);

elexpr_t *make_elexpr_range(expr_t *l1, expr_t *l2);
elexpr_t *make_elexpr(expr_t *l);

expr_t *limit_from_id(char *id);
expr_t *limit_from_signed_id(op_t op,char *id);

expr_list_t *expr_list_add(expr_list_t *new_list, expr_t *l);
var_list_t *var_list_add(var_list_t *new_list, var_t *v);
elexpr_list_t *elexpr_list_add(elexpr_list_t *new_list,elexpr_t *el);

iter_t *make_iter_space(expr_t *l1,int step,expr_t *l3);

#endif
