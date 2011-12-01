#ifndef _EXPR_TOOLBOX_H
#define _EXPR_TOOLBOX_H

#include "semantics.h"

expr_t *expr_from_variable(var_t *v);
expr_t *expr_from_STRING(char *id);
expr_t *expr_from_setexpression(elexpr_list_t *list);
expr_t *expr_from_hardcoded_int(int value);
expr_t *expr_from_signed_hardcoded_int(op_t op,int value);
expr_t *expr_from_hardcoded_real(float value);
expr_t *expr_from_hardcoded_boolean(int value);
expr_t *expr_from_hardcoded_char(char value);

expr_t *expr_from_function_call(char *id,expr_list_t *list);

expr_t *expr_from_lost_int(int value);
expr_t *expr_from_lost_boolean(int value);

expr_t *normalize_expr_set(expr_t *expr_set);
expr_t *expr_distribute_inop_to_set(expr_t *el,expr_t *expr_set);

char *op_literal(op_t op);
int check_if_boolean(expr_t *l);

#endif
