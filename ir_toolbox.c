#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"
#include "scope.h"
#include "symbol_table.h"
#include "ir.h"
#include "ir_toolbox.h"
#include "expressions.h"
#include "expr_toolbox.h"
#include "err_buff.h"

char label_buf[MAX_LABEL_SIZE];

char *new_label_literal(char *label) {
    if (!label) {
        return new_label_unique(DEFAULT_LABEL_PREFIX);
    }
    return strdup(label);
}

char *new_label_unique(char *prefix) {
    static int n = 0;

    if (!prefix) {
        prefix = DEFAULT_LABEL_PREFIX;
    }

    snprintf(label_buf,MAX_LABEL_SIZE,"L_%s_%d",prefix,n);
    n++;
    return strdup(label_buf);
}

char *new_label_true(char *branch_label) {
    if (!branch_label) {
        branch_label = DEFAULT_LABEL_PREFIX;
    }

    snprintf(label_buf,MAX_LABEL_SIZE,"%s_true",branch_label);
    return strdup(label_buf);
}

char *new_label_false(char *branch_label) {
    if (!branch_label) {
        branch_label = DEFAULT_LABEL_PREFIX;
    }

    snprintf(label_buf,MAX_LABEL_SIZE,"%s_false",branch_label);
    return strdup(label_buf);
}

char *new_label_loop_to_cond(char *branch_label) {
    if (!branch_label) {
        branch_label = DEFAULT_LABEL_PREFIX;
    }

    snprintf(label_buf,MAX_LABEL_SIZE,"%s_cond",branch_label);
    return strdup(label_buf);
}

char *new_label_exit_branch(char *branch_label) {
    if (!branch_label) {
        branch_label = DEFAULT_LABEL_PREFIX;
    }

    snprintf(label_buf,MAX_LABEL_SIZE,"%s_exit",branch_label);
    return strdup(label_buf);
}

char *new_label_subprogram(char *sub_name) {
    if (!sub_name) {
        sub_name = DEFAULT_LABEL_PREFIX;
    }

    snprintf(label_buf,MAX_LABEL_SIZE,"sub_%s",sub_name);
    return strdup(label_buf);
}

ir_node_t *expr_tree_to_ir_tree(expr_t *ltree) {
    ir_node_t *new_node;
    ir_node_t *convert_node;
    ir_node_t *new_ASM_load_stmt;
    ir_node_t *ptr_deref;
    ir_node_t *dark_init_node;
    ir_node_t *new_func_call;
    sem_t *sem_1;

    //we do not handle EXPR_STRING here, strings can be only on assignments (not inside expressions)

    if (!ltree || ltree->expr_is==EXPR_LOST) {
        return NULL;
    }

    if (ltree->expr_is==EXPR_RVAL) {
        if (ltree->op==RELOP_IN) {
            //operator 'in' applies only to one set, see expr_distribute_inop_to_set() in expr_toolbox.c
            if (ltree->l2->expr_is==EXPR_LVAL) {
                new_node = make_bitmap_inop_check(ltree);
                return new_node;
            }
            else if (ltree->l2->expr_is==EXPR_SET || ltree->l2->expr_is==EXPR_NULL_SET) {
                new_node = make_dynamic_inop_check(ltree);
                return new_node;
            }
            else {
                yyerror("UNEXPECTED_ERROR: 99-1");
                exit(EXIT_FAILURE);
            }
        }
        //{		else if (ltree->op==RELOP_B) {}
        //		else if (ltree->op==RELOP_BE) {}
        //		else if (ltree->op==RELOP_L) {}
        //		else if (ltree->op==RELOP_LE) {}
        //		else if (ltree->op==RELOP_NE) {}
        //		else if (ltree->op==RELOP_EQU) {}
        //		else if (ltree->op==OP_SIGN) {}
        //		else if (ltree->op==OP_PLUS) {}
        //		else if (ltree->op==OP_MINUS) {}
        //		else if (ltree->op==OP_MULT) {}
        //		else if (ltree->op==OP_RDIV) {}
        //		else if (ltree->op==OP_DIV) {}
        //		else if (ltree->op==OP_MOD) {}
        //		else if (ltree->op==OP_AND) {}
        //		else if (ltree->op==OP_OR) {}
        //		else if (ltree->op==OP_NOT) {}
        //}
        else { //it's all the same
            new_node = new_ir_node_t(NODE_RVAL);
            new_node->op_rval = ltree->op;
            //also convert the l1 and l2 from ltree
            new_node->ir_rval = expr_tree_to_ir_tree(ltree->l1);
            new_node->ir_rval2 = expr_tree_to_ir_tree(ltree->l2);

            if (ltree->convert_to==SEM_INTEGER) {
                convert_node = new_ir_node_t(NODE_ASM_CONVERT_TO_INT);
                convert_node->ir_rval = new_node;
                return convert_node;
            }
            else if (ltree->convert_to==SEM_REAL) {
                convert_node = new_ir_node_t(NODE_ASM_CONVERT_TO_REAL);
                convert_node->ir_rval = new_node;
                return convert_node;
            }
            return new_node;
        }
    }
    else if (ltree->expr_is==EXPR_HARDCODED_CONST) {
        new_node = new_ir_node_t(NODE_HARDCODED_RVAL);
        new_node->ival = ltree->ival;
        new_node->fval = ltree->fval;
        return new_node;
    }
    else if (ltree->expr_is==EXPR_SET || ltree->expr_is==EXPR_NULL_SET) {
        //convert to bitmap, this set goes for assignment
        new_node = create_bitmap(ltree);
        return new_node;
    }
    else if (ltree->expr_is==EXPR_LVAL) {
        if (ltree->datatype->is==TYPE_SET) {
	    //we handle differently the lvalue sets, see new_assignment() in ir.c
            printf("UNEXPECTED_ERROR:72-1");
            exit(EXIT_FAILURE);
        }

        if (ltree->var->Lvalue->content_type==PASS_VAL) {
            //ID_RETURN goes in here
            //calculate the needed base address
            new_node = new_ir_node_t(NODE_HARDCODED_LVAL);
            new_node->lval = ltree->var->Lvalue;

            //load final address with offset
            new_ASM_load_stmt = new_ir_node_t(NODE_ASM_LOAD);
            new_ASM_load_stmt->ir_lval = new_node;
            new_ASM_load_stmt->ir_rval = expr_tree_to_ir_tree(ltree->var->Lvalue->offset_expr);
            new_node = new_ASM_load_stmt;

	    if (ltree->var->id_is==ID_RETURN) {
                sem_1 = sm_find(ltree->var->name);
                dark_init_node = prepare_stack_for_call(sem_1->subprogram,ltree->expr_list);
                if (!dark_init_node) {
                    //parse errors, omit code
                    return NULL;
                }

                new_func_call = jump_and_link_to(sem_1->subprogram->label);
                new_func_call = link_stmt_to_stmt(new_func_call,dark_init_node);

                //finally we must read the return value after the actual call
                new_node = link_stmt_to_stmt(new_node,new_func_call);
	    }
        }
        else { //PASS_REF (only for formal parameters inside subprograms)
            //calculate the reference address
            ptr_deref = new_ir_node_t(NODE_HARDCODED_LVAL);
            ptr_deref->lval = ltree->var->Lvalue;

            //load the needed base address
            new_ASM_load_stmt = new_ir_node_t(NODE_ASM_LOAD);
            new_ASM_load_stmt->ir_lval = ptr_deref;
            new_ASM_load_stmt->ir_rval = NULL; //we don't have offset here

            //load the final address
            new_node = new_ir_node_t(NODE_ASM_LOAD);
            new_node->ir_lval = new_ASM_load_stmt;
            new_node->ir_rval = expr_tree_to_ir_tree(ltree->var->Lvalue->offset_expr);
        }

        if (ltree->convert_to==SEM_INTEGER) {
            convert_node = new_ir_node_t(NODE_ASM_CONVERT_TO_INT);
            convert_node->ir_rval = new_node;
            return convert_node;
        }
        else if (ltree->convert_to==SEM_REAL) {
            convert_node = new_ir_node_t(NODE_ASM_CONVERT_TO_REAL);
            convert_node->ir_rval = new_node;
            return convert_node;
        }
        return new_node;
    }
    else {
        yyerror("UNEXPECTED_ERROR: 99-2");
        exit(EXIT_FAILURE);
    }
}

ir_node_t *calculate_lvalue(var_t *v) {
    ir_node_t *new_ir_lval;
    ir_node_t *new_node;
    ir_node_t *ptr_deref;

    if (!v) {
        printf("UNEXPECTED_ERROR: 74-1\n");
        exit(EXIT_FAILURE);
    }

    //calculate the reference address
    new_node = new_ir_node_t(NODE_HARDCODED_LVAL);
    new_node->lval = v->Lvalue;

    ptr_deref = NULL;
    if (v->Lvalue->content_type==PASS_REF) {
        //load the final address
        ptr_deref = new_ir_node_t(NODE_ASM_LOAD);
        ptr_deref->ir_lval = new_node;
        ptr_deref->ir_rval = NULL; //we don't have offset here
    }

    //load final address with offset
    new_ir_lval = new_ir_node_t(NODE_LVAL);
    new_ir_lval->ir_lval = (ptr_deref)?ptr_deref:new_node;
    new_ir_lval->ir_rval = expr_tree_to_ir_tree(v->Lvalue->offset_expr); //offset is NULL for ID_RETURN

    return new_ir_lval;
}

expr_t *make_enum_subset_bound_checks(var_t *v,expr_t *l) {
    //the variable is an enumeration or a subset
    expr_t *left_bound;
    expr_t *right_bound;

    expr_t *left_cond;
    expr_t *right_cond;
    expr_t *total_cond;

    left_bound = expr_from_hardcoded_int(v->datatype->enum_num[0]); //the left (small) bound
    right_bound = expr_from_hardcoded_int(v->datatype->enum_num[v->datatype->field_num-1]); //the right (big) bound

    left_cond = expr_relop_equ_addop_mult(left_bound,RELOP_LE,l);
    right_cond = expr_relop_equ_addop_mult(l,RELOP_LE,right_bound);
    total_cond = expr_orop_andop_notop(left_cond,OP_AND,right_cond);

    return total_cond;
}

expr_t *make_ASCII_bound_checks(var_t *v,expr_t *l) {
    //the variable is a char
    expr_t *left_bound;
    expr_t *right_bound;

    expr_t *left_cond;
    expr_t *right_cond;
    expr_t *total_cond;

    left_bound = expr_from_hardcoded_int(0); //the left (small) bound
    right_bound = expr_from_hardcoded_int(0x7f); //the right (big) bound

    left_cond = expr_relop_equ_addop_mult(left_bound,RELOP_LE,l);
    right_cond = expr_relop_equ_addop_mult(l,RELOP_LE,right_bound);
    total_cond = expr_orop_andop_notop(left_cond,OP_AND,right_cond);

    return total_cond;
}

ir_node_t *prepare_stack_for_call(func_t *subprogram, expr_list_t *list) {
    ir_node_t *new_stack_init_node;
    ir_node_t *tmp_assign;

    data_t *el_datatype;
    var_t *tmp_var;

    int i;

    if (!list && subprogram->param_num!=0) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("ERROR: null expr_list for subprogram call (debugging info)");
#endif
        return NULL;
    }

    if (list->all_expr_num != subprogram->param_num) {
        sprintf(str_err,"ERROR: invalid number of parameters: subprogram `%s` takes %d parameters",subprogram->func_name,subprogram->param_num);
        yyerror(str_err);
        return NULL;
    }

    if (MAX_EXPR_LIST - list->expr_list_empty == subprogram->param_num) {
        new_stack_init_node = NULL;
        tmp_var = (var_t*)malloc(sizeof(var_t)); //allocate once
        for (i=0;i<subprogram->param_num;i++) {
            tmp_assign = NULL;

            if (list->expr_list[i]->expr_is==EXPR_LOST) {
                //parse errors
                free(tmp_var);
                return NULL;
            }

            tmp_var->id_is = ID_VAR;
            tmp_var->datatype = subprogram->param[i]->datatype;
            tmp_var->name = subprogram->param[i]->name;
            tmp_var->scope = get_current_scope();
            tmp_var->Lvalue = subprogram->param_Lvalue[i];
            tmp_var->cond_assign = NULL;

            el_datatype = list->expr_list[i]->datatype;

            if (subprogram->param[i]->pass_mode==PASS_REF) {
                //accept only lvalues
                if (list->expr_list[i]->expr_is!=EXPR_LVAL) {
                    sprintf(str_err,"ERROR: parameter '%s' must be variable to be passed by reference",subprogram->param[i]->name);
                    yyerror(str_err);
                    free(tmp_var);
                    return NULL;
                }

                if (el_datatype != subprogram->param[i]->datatype) {
                    free(tmp_var);
                    return NULL;
                }

                if (list->expr_list[i]->var->id_is==ID_VAR_GUARDED) {
                    sprintf(str_err,"ERROR: guard variable of for_statement '%s' passed by refference",list->expr_list[i]->var->name);
                    yyerror(str_err);
                    free(tmp_var);
                    return NULL;
                }
            }
            else { //PASS_VAL
                //accept anything, if it's not rvalue we pass it's address
                if (TYPE_IS_COMPOSITE(el_datatype)) {
                    yyerror("ERROR: arrays, records and set datatypes can only be passed by refference");
                    free(tmp_var);
                    return NULL;
                }
            }
#warning "if errors, new_assign_stmt() leaks memory"
	    tmp_assign = new_assign_stmt(tmp_var,list->expr_list[i]);
            new_stack_init_node = link_stmt_to_stmt(tmp_assign,new_stack_init_node);
        }
        free(tmp_var);
        return new_stack_init_node;
    }

    //some expressions are invalid, but their number is correct, avoid some unreal error messages afterwards
    return NULL;
}

var_t *new_normal_variable_from_guarded(var_t *guarded) {
    var_t *new_var;

    if (!guarded) {
        yyerror("ERROR: null guarded variable for conversion (debugging info)");
        return NULL;
    }

    if (guarded->id_is!=ID_VAR_GUARDED) {
        yyerror("INTERNAL_ERROR: variable is not a guard variable, check 'new_for_statement()'");
        return NULL;
    }

    new_var = (var_t*)malloc(sizeof(var_t));
    new_var->id_is = ID_VAR;
    new_var->datatype = guarded->datatype;
    new_var->name = guarded->name;
    new_var->scope = guarded->scope;
    new_var->Lvalue = guarded->Lvalue;

    return new_var;
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

ir_node_t *create_bitmap(expr_t *expr_set) {
    op_t op;
    var_t *tmp;

    ir_node_t *new_ASM_binary;
    ir_node_t *bitmap1;
    ir_node_t *bitmap2;
    ir_node_t *ir_final;
    //	ir_node_t *tmp_node;

    if (!expr_set) {
        yyerror("UNEXPECTED_ERROR: 72-1");
        exit(EXIT_FAILURE);
    }

    expr_set = normalize_expr_set(expr_set);

    if (!(expr_set->expr_is==EXPR_LVAL && expr_set->datatype->is==TYPE_SET) &&
        expr_set->expr_is!=EXPR_NULL_SET && expr_set->expr_is!=EXPR_SET) {
        yyerror("UNEXPECTED_ERROR: 72-3");
        exit(EXIT_FAILURE);
    }

    if (expr_set->parent==expr_set->parent) {
        //this is the root of the tree

        op = expr_set->op;

        //check first if the set is basic
        if (op==OP_IGNORE) {
            //result goes to x
            return create_basic_bitmap(x,expr_set);
        }
        //call the bitmap_generator

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
        //tmp_node = NULL;
        if (op==OP_PLUS) {
            new_ASM_binary = new_ir_node_t(NODE_TYPEOF_SET_OR);
            new_ASM_binary->ir_lval = calculate_lvalue(ll);
            new_ASM_binary->ir_lval2 = calculate_lvalue(rr);
            new_ASM_binary->ir_lval_dest = calculate_lvalue(x);
            return new_ASM_binary;
        }
        else if (op==OP_MINUS) {
            //should never reach here, we cannod assign an expression of set datatype
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
            yyerror("UNEXPECTED_ERROR: 71-2");
            exit(EXIT_FAILURE);
        }

        ir_final = NULL;
        ir_final = link_stmt_to_stmt(bitmap1,ir_final);
        ir_final = link_stmt_to_stmt(bitmap2,ir_final);
        //ir_final = link_stmt_to_stmt(tmp_node,ir_final); //maybe tmp_node is NULL, it is ignored
        ir_final = link_stmt_to_stmt(new_ASM_binary,ir_final);
    }

    yyerror("UNEXPECTED_ERROR: 71-3");
    exit(EXIT_FAILURE);
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
    else {
        tmp_node = NULL;
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
        ir_final = link_stmt_to_stmt(tmp_node,ir_final); //if tmp_node is NULL, it is ignored
        ir_final = link_stmt_to_stmt(new_ASM_binary,ir_final);
    }

    yyerror("UNEXPECTED_ERROR: make_bitmap() __recursion");
    exit(EXIT_FAILURE);
}

ir_node_t *create_basic_bitmap(var_t *factory,expr_t *expr_set) {
    //return the lvalue which contains the new bitmap
    int i;
    ir_node_t *ir_final;
    ir_node_t *tmp_node;

    expr_t *normalized_left;
    expr_t *normalized_right;
    expr_t *norm_base;
    expr_t *left;
    expr_t *right;
    ir_node_t *dark_init_node;

    if (expr_set->expr_is==EXPR_LVAL) {
        //load from memory
        return calculate_lvalue(expr_set->var);
    }

    //init NULL set, every time
    ir_final = new_ir_node_t(NODE_INIT_NULL_SET);
    ir_final->lval = factory->Lvalue;

    if (expr_set->expr_is==EXPR_SET) {
        if (!TYPE_IS_STANDARD(expr_set->datatype)) {
            //elements are enumerations, normalize them
            norm_base = expr_from_hardcoded_int(expr_set->datatype->enum_num[0]);
            for(i=0;i<MAX_SET_ELEM-expr_set->elexpr_list->elexpr_list_empty;i++) {
                left = expr_set->elexpr_list->elexpr_list[i]->left;
                right = expr_set->elexpr_list->elexpr_list[i]->right;

                if (left==right) {
                    //one element
                    normalized_left = expr_relop_equ_addop_mult(left,OP_MINUS,norm_base);
                    dark_init_node = expr_tree_to_ir_tree(normalized_left);
                    tmp_node = new_ir_node_t(NODE_ADD_ELEM_TO_SET);
                    tmp_node->ir_rval = dark_init_node;
                }
                else {
                    //range of elements
                    tmp_node = new_ir_node_t(NODE_ADD_ELEM_RANGE_TO_SET);

                    normalized_left = expr_relop_equ_addop_mult(left,OP_MINUS,norm_base);
                    dark_init_node = expr_tree_to_ir_tree(normalized_left);
                    tmp_node->ir_rval = dark_init_node;

                    normalized_right = expr_relop_equ_addop_mult(right,OP_MINUS,norm_base);
                    dark_init_node = expr_tree_to_ir_tree(normalized_right);
                    tmp_node->ir_rval2 = dark_init_node;
                }
                ir_final = link_stmt_to_stmt(tmp_node,ir_final);
            }
        }
        else {
            //elements are char or boolean
            for(i=0;i<MAX_SET_ELEM-expr_set->elexpr_list->elexpr_list_empty;i++) {
                left = expr_set->elexpr_list->elexpr_list[i]->left;
                right = expr_set->elexpr_list->elexpr_list[i]->right;

                if (left==right) {
                    //one element
                    tmp_node = new_ir_node_t(NODE_ADD_ELEM_TO_SET);
                    tmp_node->ir_rval = expr_tree_to_ir_tree(left);
                }
                else {
                    //range of elements
                    tmp_node = new_ir_node_t(NODE_ADD_ELEM_RANGE_TO_SET);
                    tmp_node->ir_rval = expr_tree_to_ir_tree(left);
                    tmp_node->ir_rval2 = expr_tree_to_ir_tree(right);
                }
                ir_final = link_stmt_to_stmt(tmp_node,ir_final);
            }
        }
    }

    tmp_node = calculate_lvalue(factory);
    ir_final = link_stmt_to_stmt(tmp_node,ir_final);
    return ir_final;
}

int check_assign_similar_comp_datatypes(data_t* vd, data_t* ld){
    int i;

    //explicit datatype matching
    if (vd==ld) {
        return 1;
    }

    //compatible datatype matching
    if (TYPE_IS_STRING(vd) && TYPE_IS_STRING(ld)) {
        return 1;
    }
    else if (TYPE_IS_ARITHMETIC(vd) && TYPE_IS_ARITHMETIC(ld)) {
        return 1;
    }
    else if (vd->is==TYPE_SUBSET) {
        return check_assign_similar_comp_datatypes(vd->def_datatype,ld);
    }
    else if (vd->is==ld->is && vd->is==TYPE_ARRAY) {
        return check_assign_similar_comp_datatypes(vd->def_datatype,ld->def_datatype);
    }
    else if (vd->is==TYPE_SET) {
        //do not recurse here, we want only explicit datatype matching
        if (vd->def_datatype==ld) {
            return 1;
        }
        return 0;
    }
    else if (vd->is==ld->is && vd->is==TYPE_RECORD) {
        if (vd->field_num==ld->field_num) {
            for(i=0;i<vd->field_num;i++) {
                if (!check_assign_similar_comp_datatypes(vd->field_datatype[i],ld->field_datatype[i])) {
                    return 0;
                }
            }
            return 1;
        }
        return 0;
    }
    else {
        return 0;
    }
}
