#ifndef _BITMAP_H
#define _BITMAP_H

#include "semantics.h"
#include "ir.h"

//we convert sets to bitmaps here
extern var_t *ll;	//left result
extern var_t *rr;	//right result
extern var_t *x;	//protected result

void init_bitmap();
ir_node_t *create_bitmap(expr_t *expr_set);
ir_node_t *create_basic_bitmap(var_t *factory,expr_t *expr_set);
ir_node_t *bitmap_generator(var_t *factory,expr_t *expr_set);

ir_node_t *make_bitmap_inop_check(expr_t *expr_inop);
ir_node_t *make_dynamic_inop_check(expr_t *expr_inop);

#endif
