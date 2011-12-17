#include <stdio.h>
#include <stdlib.h>

#include "bitmap.h"
#include "expressions.h"
#include "expr_toolbox.h"
#include "ir_toolbox.h"
#include "mem_reg.h"
#include "symbol_table.h"
#include "err_buff.h"

data_t datatype_max_sizeof_set; //array of chars with 16 elements (max size of sets in bytes)

var_t *ll;	//left result
var_t *rr;	//right result
var_t *x;	//protected result

void init_bitmap() {
    //init bitmap_factory, do NOT put this datatype in symbol table
    datatype_max_sizeof_set.is = TYPE_ARRAY;
    datatype_max_sizeof_set.def_datatype = SEM_CHAR;
    datatype_max_sizeof_set.data_name = "__internal_bitmap_factory_datatype__";
    datatype_max_sizeof_set.memsize = 16*MEM_SIZEOF_CHAR; //we use only this

    //these go to gp (we are in main scope)
    ll = (var_t*)malloc(sizeof(var_t));
    ll->id_is = ID_VAR;
    ll->name = "__internal_bitmap_ll_";
    ll->datatype = &datatype_max_sizeof_set;
    ll->scope = 0; //we don't use it
    ll->Lvalue = mem_allocate_symbol(&datatype_max_sizeof_set);

    rr = (var_t*)malloc(sizeof(var_t));
    rr->id_is = ID_VAR;
    rr->name = "__internal_bitmap_rr__";
    rr->datatype = &datatype_max_sizeof_set;
    rr->scope = 0; //we don't use it
    rr->Lvalue = mem_allocate_symbol(&datatype_max_sizeof_set);

    x = (var_t*)malloc(sizeof(var_t));
    x->id_is = ID_VAR;
    x->name = "__internal_bitmap_x__";
    x->datatype = &datatype_max_sizeof_set;
    x->scope = 0; //we don't use it
    x->Lvalue = mem_allocate_symbol(&datatype_max_sizeof_set);
}

ir_node_t *create_bitmap(expr_t *expr_set) {
    op_t op;
    var_t *tmp;

    ir_node_t *new_ASM_binary;
    ir_node_t *bitmap1;
    ir_node_t *bitmap2;
    ir_node_t *ir_final;

    if (!expr_set || expr_set->expr_is==EXPR_LOST) {
        yyerror("UNEXPECTED_ERROR: 72-1");
        exit(EXIT_FAILURE);
    }

    if (expr_set->expr_is==EXPR_LVAL && expr_set->datatype->is==TYPE_SET) {
        return calculate_lvalue(expr_set->var);
    }

    if (expr_set->expr_is==EXPR_NULL_SET) {
        ir_final = new_ir_node_t(NODE_INIT_NULL_SET);
        ir_final->ir_lval = calculate_lvalue(x); //create the NULL set at factory 'x'
    }

    if (expr_set->expr_is!=EXPR_SET) {
        yyerror("UNEXPECTED_ERROR: 72-2");
        exit(EXIT_FAILURE);
    }

    normalize_expr_set(expr_set);

    op = expr_set->op;

    //check first if the set is basic
    if (op==OP_IGNORE) {
        //result goes to x
        //in this case the bitmap_generator() calls immediately the create_basic_bitmap()
        //but we want the later to be known only from the bitmap_generator()
        return bitmap_generator(x,expr_set);
    }

    //else
    //call the bitmap_generator for the child sets

    //run left subtree
    bitmap1 = bitmap_generator(ll,expr_set->l1);

    //swap (lr,x)
    tmp = x;
    x = ll;
    ll = tmp;

    //run right subtree
    bitmap2 = bitmap_generator(rr,expr_set->l2);

    //swap (lr,x)
    tmp = x;
    x = ll;
    ll = tmp;

    //result goes to x
    if (op==OP_PLUS) {
        new_ASM_binary = new_ir_node_t(NODE_TYPEOF_SET_OR);
        new_ASM_binary->ir_lval = calculate_lvalue(ll);
        new_ASM_binary->ir_lval2 = calculate_lvalue(rr);
        new_ASM_binary->ir_lval_dest = calculate_lvalue(x);
        return new_ASM_binary;
    }
    else if (op==OP_MINUS) {
        //should never reach here, we cannot assign an expression of set datatype
        yyerror("UNEXPECTED_ERROR: 71-1");
        exit(EXIT_FAILURE);
        /*
          tmp_node = new_ir_node_t(NODE_TYPEOF_SET_NOT);
          tmp_node->ir_lval = calculate_lvalue(rr);
          tmp_node->ir_lval_dest = calculate_lvalue(rr);
          new_ASM_binary = new_ir_node_t(NODE_TYPEOF_SET_AND);
          new_ASM_binary->ir_lval = calculate_lvalue(lr);
          new_ASM_binary->ir_lval2 = calculate_lvalue(rr);
          new_ASM_binary->ir_lval_dest = calculate_lvalue(x);
          return new_ASM_binary;
        */
    }
    else if (op==OP_MULT) {
        new_ASM_binary = new_ir_node_t(NODE_TYPEOF_SET_AND);
        new_ASM_binary->ir_lval = calculate_lvalue(ll);
        new_ASM_binary->ir_lval2 = calculate_lvalue(rr);
        new_ASM_binary->ir_lval_dest = calculate_lvalue(x);
        return new_ASM_binary;
    }
    else {
        yyerror("UNEXPECTED_ERROR: 71-2 :bad operator in create_bitmap()");
        exit(EXIT_FAILURE);
    }

    ir_final = NULL;
    ir_final = link_stmt_to_stmt(bitmap1,ir_final);
    ir_final = link_stmt_to_stmt(bitmap2,ir_final);
    ir_final = link_stmt_to_stmt(new_ASM_binary,ir_final);

    return ir_final;
}

ir_node_t *bitmap_generator(var_t *factory,expr_t *expr_set) {
    //return the lvalue which contains the new bitmap
    //remember the bitmap_left bitmap_right bitmap_result in ir.c
    op_t op;
    var_t *tmp;

    ir_node_t *new_ASM_binary;
    ir_node_t *bitmap1;
    ir_node_t *bitmap2;
    ir_node_t *tmp_node;
    ir_node_t *ir_final;

    op = expr_set->op;

    if (op==OP_IGNORE) {
        return create_basic_bitmap(factory,expr_set);
    }

    //first the left bitmap (left child)
    bitmap1 = bitmap_generator(ll,expr_set->l1);

    //swap (lr,x), protect lr
    tmp = x;
    x = ll;
    ll = tmp;

    bitmap2 = bitmap_generator(rr,expr_set->l2);

    //swap (lr,x), restore lr
    tmp = x;
    x = ll;
    ll = tmp;

    if (op==OP_PLUS) {
        new_ASM_binary = new_ir_node_t(NODE_TYPEOF_SET_OR);
        new_ASM_binary->ir_lval = calculate_lvalue(ll);
        new_ASM_binary->ir_lval2 = calculate_lvalue(rr);
        new_ASM_binary->ir_lval_dest = calculate_lvalue(factory);
    }
    else if (op==OP_MINUS) {
        tmp_node = new_ir_node_t(NODE_TYPEOF_SET_NOT);
        tmp_node->ir_lval = calculate_lvalue(rr);
        tmp_node->ir_lval_dest = calculate_lvalue(rr);
        new_ASM_binary = new_ir_node_t(NODE_TYPEOF_SET_AND);
        new_ASM_binary->ir_lval = calculate_lvalue(ll);
        new_ASM_binary->ir_lval2 = calculate_lvalue(rr);
        new_ASM_binary->ir_lval_dest = calculate_lvalue(factory);

        new_ASM_binary = link_stmt_to_stmt(new_ASM_binary,tmp_node);
    }
    else if (op==OP_MULT) {
        new_ASM_binary = new_ir_node_t(NODE_TYPEOF_SET_AND);
        new_ASM_binary->ir_lval = calculate_lvalue(ll);
        new_ASM_binary->ir_lval2 = calculate_lvalue(rr);
        new_ASM_binary->ir_lval_dest = calculate_lvalue(factory);
    }

    ir_final = NULL;
    ir_final = link_stmt_to_stmt(bitmap1,ir_final);
    ir_final = link_stmt_to_stmt(bitmap2,ir_final);
    ir_final = link_stmt_to_stmt(new_ASM_binary,ir_final);

    return ir_final;
}

ir_node_t *create_basic_bitmap(var_t *factory,expr_t *expr_set) {
    //return the lvalue which contains the new bitmap
    int i;
    ir_node_t *ir_final;
    ir_node_t *tmp_node;

    expr_t *norm_base;
    expr_t *left;
    expr_t *right;

    if (expr_set->expr_is==EXPR_LVAL) {
        ir_final = new_ir_node_t(NODE_ASSIGN_SET);
        ir_final->ir_lval = calculate_lvalue(expr_set->var);
        ir_final->ir_lval_dest = calculate_lvalue(factory);
        return ir_final;
    }

    //init NULL set, every time
    ir_final = new_ir_node_t(NODE_INIT_NULL_SET);
    ir_final->ir_lval_dest = calculate_lvalue(factory);

    if (expr_set->expr_is==EXPR_SET) {
        //elements are enumerations, normalize them
        norm_base = expr_from_hardcoded_int(expr_set->datatype->enum_num[0]);
        for(i=0;i<MAX_SET_ELEM-expr_set->elexpr_list->elexpr_list_empty;i++) {
            left = expr_set->elexpr_list->elexpr_list[i]->left;
            right = expr_set->elexpr_list->elexpr_list[i]->right;

            if (!TYPE_IS_STANDARD(expr_set->datatype)) {
                //enumeration or subset
                //normalize left (we always need left)
                left = expr_relop_equ_addop_mult(left,OP_MINUS,norm_base);
                //normalize right
                right = expr_relop_equ_addop_mult(right,OP_MINUS,norm_base);
            }

            if (left==right) {
                //one element, use left
                tmp_node = new_ir_node_t(NODE_ADD_ELEM_TO_SET);
                tmp_node->ir_rval = expr_tree_to_ir_tree(left);
            }
            else {
                //range of elements
                tmp_node = new_ir_node_t(NODE_ADD_ELEM_RANGE_TO_SET);
                tmp_node->ir_rval = expr_tree_to_ir_tree(left);
                tmp_node->ir_rval2 = expr_tree_to_ir_tree(right);
            }

            tmp_node->ir_lval_dest = calculate_lvalue(factory);
            ir_final = link_stmt_to_stmt(tmp_node,ir_final);
        }
    }

    return ir_final;
}

void normalize_expr_set(expr_t* expr_set){
    expr_t *new_expr;
    expr_t *tmp1;
    expr_t *tmp2;
    expr_t *tmp3;
    expr_t *tmp4;
    expr_t *expr1;
    expr_t *expr2;

    expr_t *l1;
    op_t op;
    expr_t *l2;

    //we have only EXPR_SET here, see create_bitmap() in ir_toolbox.c
    //and op is OP_PLUS, OP_MINUS, OP_MULT
    //OP_IGNORE here means a set in a leaf node (end of recursion)

    //transform set from (a+b)*(c-d)
    //              to   a*c -a*d + b*c -b*d

    if (!expr_set) {
        return;
    }

    l1 = expr_set->l1;
    op = expr_set->op;
    l2 = expr_set->l2;

    if (op==OP_IGNORE) {
        //nothing to do
        return;
    }

    if (op==OP_MULT) {
        if (l1->op==OP_IGNORE && l2->op==OP_IGNORE) {
            return;
        }

        if (l1->op==OP_IGNORE) {
            tmp1 = expr_muldivandop(l1,op,l2->l1);
            tmp2 = expr_muldivandop(l1,op,l2->l2);
            expr_set = expr_relop_equ_addop_mult(tmp1,l2->op,tmp2);
        }

        if (l2->op==OP_IGNORE) {
            tmp1 = expr_muldivandop(l1->l1,op,l2);
            tmp2 = expr_muldivandop(l1->l2,op,l2);
            expr_set = expr_relop_equ_addop_mult(tmp1,l1->op,tmp2);
        }

        if ((l1->op!=OP_MULT && l1->op!=OP_IGNORE) && (l2->op!=OP_MULT && l2->op!=OP_IGNORE)) {
            tmp1 = expr_muldivandop(l1->l1,op,l2->l1);
            tmp2 = expr_muldivandop(l1->l1,op,l2->l2);
            tmp3 = expr_muldivandop(l1->l2,op,l2->l1);
            tmp4 = expr_muldivandop(l1->l2,op,l2->l2);
            expr1 = expr_relop_equ_addop_mult(tmp1,l2->op,tmp2);
            expr2 = expr_relop_equ_addop_mult(tmp3,l2->op,tmp4);
            new_expr = expr_relop_equ_addop_mult(expr1,l1->op,expr2);
        }
        else if (l1->op!=OP_MULT && l1->op!=OP_IGNORE) {
            normalize_expr_set(l2);
            tmp1 = expr_muldivandop(l1->l1,op,expr2);
            tmp2 = expr_muldivandop(l1->l2,op,expr2);
            new_expr = expr_relop_equ_addop_mult(tmp1,l1->op,tmp2);
        }
        else {
            normalize_expr_set(l1);
            tmp1 = expr_muldivandop(expr1,op,l2->l1);
            tmp2 = expr_muldivandop(expr1,op,l2->l2);
            new_expr = expr_relop_equ_addop_mult(tmp1,l2->op,tmp2);
        }
        return;
    }
    yyerror("UNEXPECTED_ERROR: 85-1");
    exit(EXIT_FAILURE);
}

ir_node_t *make_bitmap_inop_check(expr_t *expr_inop) {
    ir_node_t *dark_init_node;
    ir_node_t *new_stmt;

    expr_t *normalized_element;
    expr_t *norm_base;

    if (!TYPE_IS_STANDARD(expr_inop->l2->datatype)) {
        norm_base = expr_from_hardcoded_int(expr_inop->l2->datatype->enum_num[0]);
        normalized_element = expr_relop_equ_addop_mult(expr_inop->l1,OP_MINUS,norm_base);
        dark_init_node = expr_tree_to_ir_tree(normalized_element);
    }
    else {
        dark_init_node = expr_tree_to_ir_tree(expr_inop->l1);
    }

    new_stmt = new_ir_node_t(NODE_CHECK_INOP_BITMAPPED);
    new_stmt->lval = expr_inop->l2->var->Lvalue; //l2 is lvalue for sure
    new_stmt->ir_rval = dark_init_node;

    return new_stmt;
}

ir_node_t *make_dynamic_inop_check(expr_t *expr_inop) {
    int i;
    int expr_num;

    expr_t *element;
    expr_t *expr_set;

    expr_t *total_cond;
    expr_t *tmp_cond;
    expr_t *left_cond;
    expr_t *right_cond;
    expr_t *left;
    expr_t *right;

    if (!expr_inop || !expr_inop->l1 || expr_inop->l2->datatype->is!=TYPE_SET) {
        return NULL; //parse errors
    }

    element = expr_inop->l1;
    expr_set = expr_inop->l2;

    total_cond = expr_from_hardcoded_boolean(0); //dummy boolean init
    expr_num = MAX_SET_ELEM - expr_set->elexpr_list->elexpr_list_empty;
    for(i=0;i<expr_num;i++) {
        left = expr_set->elexpr_list->elexpr_list[i]->left;
        right = expr_set->elexpr_list->elexpr_list[i]->right;
        if (left==right) {
            tmp_cond = expr_relop_equ_addop_mult(left,RELOP_EQU,element);
        }
        else {
            // left <= element <= right
            left_cond = expr_relop_equ_addop_mult(left,RELOP_LE,element);
            right_cond = expr_relop_equ_addop_mult(element,RELOP_LE,right);
            tmp_cond = expr_orop_andop_notop(left_cond,OP_AND,right_cond);
        }
        total_cond = expr_orop_andop_notop(total_cond,OP_OR,tmp_cond);
    }
    return expr_tree_to_ir_tree(total_cond);
}
