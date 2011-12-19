#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantics.h"
#include "scope.h"
#include "symbol_table.h"
#include "ir.h"
#include "ir_toolbox.h"
#include "bitmap.h"
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
                convert_node = new_ir_node_t(NODE_CONVERT_TO_INT);
                convert_node->ir_rval = new_node;
                return convert_node;
            }
            else if (ltree->convert_to==SEM_REAL) {
                convert_node = new_ir_node_t(NODE_CONVERT_TO_REAL);
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

        new_node = new_ir_node_t(NODE_LOAD);
        new_node->address = calculate_lvalue(ltree->var);

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

        if (ltree->convert_to==SEM_INTEGER) {
            convert_node = new_ir_node_t(NODE_CONVERT_TO_INT);
            convert_node->ir_rval = new_node;
            return convert_node;
        }
        else if (ltree->convert_to==SEM_REAL) {
            convert_node = new_ir_node_t(NODE_CONVERT_TO_REAL);
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
    ir_node_t *new_node;
    ir_node_t *ptr_deref;

    expr_t *address_expr;

    if (!v) {
        printf("UNEXPECTED_ERROR: 74-1\n");
        exit(EXIT_FAILURE);
    }

    //calculate the reference address
    new_node = new_ir_node_t(NODE_HARDCODED_LVAL);
    new_node->lval = v->Lvalue;

    if (v->Lvalue->content_type==PASS_REF) {
        //set the address so far
        new_node->address = expr_tree_to_ir_tree(v->Lvalue->seg_offset);

        //load the final address
        ptr_deref = new_ir_node_t(NODE_LOAD);
        ptr_deref->address = new_node;

        new_node = new_ir_node_t(NODE_LVAL);
        new_node->address = ptr_deref;

        //put the offset separate after the load
        new_node->offset = expr_tree_to_ir_tree(v->Lvalue->offset_expr);
    } else { //PASS_VAL
        //put the offset together with the *address
        address_expr = expr_relop_equ_addop_mult(v->Lvalue->seg_offset,OP_PLUS,v->Lvalue->offset_expr);
        new_node->address = expr_tree_to_ir_tree(address_expr);
    }

    return new_node;
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
        sprintf(str_err,"invalid number of parameters: subprogram `%s` takes %d parameters",subprogram->func_name,subprogram->param_num);
        yyerror(str_err);
        return NULL;
    }

    if (MAX_EXPR_LIST - list->expr_list_empty == subprogram->param_num) {
        new_stack_init_node = NULL;

        tmp_var = (var_t*)malloc(sizeof(var_t)); //allocate once
        tmp_var->id_is = ID_VAR;
        tmp_var->scope = get_current_scope();
        tmp_var->cond_assign = NULL;

        for (i=0;i<subprogram->param_num;i++) {
            tmp_assign = NULL;

            if (list->expr_list[i]->expr_is==EXPR_LOST) {
                //parse errors
                free(tmp_var);
                return NULL;
            }

            tmp_var->datatype = subprogram->param[i]->datatype;
            tmp_var->name = subprogram->param[i]->name;
            tmp_var->Lvalue = subprogram->param_Lvalue[i];

            el_datatype = list->expr_list[i]->datatype;

            if (subprogram->param[i]->pass_mode==PASS_REF) {
                //accept only lvalues
                if (list->expr_list[i]->expr_is!=EXPR_LVAL) {
                    sprintf(str_err,"parameter '%s' must be variable to be passed by reference",subprogram->param[i]->name);
                    yyerror(str_err);
                    free(tmp_var);
                    return NULL;
                }

                if (el_datatype != subprogram->param[i]->datatype) {
                    sprintf(str_err,"passing refference to datatype '%s' with datatype '%s'",subprogram->param[i]->datatype->data_name,el_datatype->data_name);
                    yyerror(str_err);
                    free(tmp_var);
                    return NULL;
                }

                if (list->expr_list[i]->var->id_is==ID_VAR_GUARDED) {
                    sprintf(str_err,"guard variable of for_statement '%s' passed by refference",list->expr_list[i]->var->name);
                    yyerror(str_err);
                    free(tmp_var);
                    return NULL;
                }

                //mark temporarily the lvalue as PASS_VAL, in order for the calculate_lvalue() to work properly
                //this is a very ugly way of doing things :( //FIXME
                tmp_var->Lvalue->content_type = PASS_VAL;

                //reminder: we are talking about stack Lvalues
                //there are no offsets, parameters do not change position in stack
                //this means that the address is HARDCODED_LVAL

                tmp_assign = new_ir_node_t(NODE_ASSIGN);
                tmp_assign->address = calculate_lvalue(tmp_var);
                tmp_assign->ir_rval = calculate_lvalue(list->expr_list[i]->var);

                //restore the PASS_REF content_type
                tmp_var->Lvalue->content_type = PASS_REF;
            }
            else { //PASS_VAL
                //accept anything, if it's not rvalue we pass it's address
                if (TYPE_IS_COMPOSITE(el_datatype)) {
                    yyerror("arrays, records and set datatypes can only be passed by refference");
                    free(tmp_var);
                    return NULL;
                }
                tmp_assign = new_assign_stmt(tmp_var,list->expr_list[i]);
            }
#warning "if errors, we leak memory"

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
        yyerror("null guarded variable for conversion (debugging info)");
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
                if (!check_assign_similar_comp_datatypes(vd->field_datatype[i],
                                                         ld->field_datatype[i])) {
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
