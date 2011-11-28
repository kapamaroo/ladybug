#ifndef _EXPRESSIONS_H
#define _EXPRESSIONS_H

#include "semantics.h"

expr_t *expr_relop_equ_addop_mult(expr_t *l1,op_t op,expr_t *l2);
expr_t *expr_inop(expr_t *l1,op_t op,expr_t *l2);
expr_t *expr_orop_andop_notop(expr_t *l1,op_t op,expr_t *l2);
expr_t *expr_muldivandop(expr_t *l1,op_t op,expr_t *l2);
expr_t *expr_sign(op_t op,expr_t *l);

expr_t *expr_mark_paren(expr_t *l);
void expr_unmark_paren(expr_t *l);
#endif
