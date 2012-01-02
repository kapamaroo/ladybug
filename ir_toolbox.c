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
#include "reg.h"

ir_node_t *ir_move_reg(reg_t *reg) {
    ir_node_t *arch_node;
    ir_node_t *arch_node2;
    ir_node_t *move_node;

    arch_node = new_ir_node_t(NODE_RVAL_ARCH);
    arch_node->reg = &R_zero;

    arch_node2 = new_ir_node_t(NODE_RVAL_ARCH);
    arch_node2->reg = reg;

    move_node = new_ir_node_t(NODE_RVAL);
    move_node->op_rval = OP_PLUS;
    move_node->ir_rval = arch_node;
    move_node->ir_rval2 = arch_node2;

    //consider integer
    move_node->data_is = TYPE_INT;

    return move_node;
}

op_t op_invert_cond(op_t op) {
    //invert the check

    switch (op) {
    case RELOP_B:       // '>'
        return RELOP_LE;
    case RELOP_BE:      // '>='
        return RELOP_L;
    case RELOP_L:       // '<'
        return RELOP_BE;
    case RELOP_LE:      // '<='
        return RELOP_B;
    case RELOP_NE:      // '<>'
        return RELOP_EQU;
    case RELOP_EQU:     // '='
        return RELOP_NE;
    case OP_AND:       	// 'and'
    	return OP_OR;
    case OP_OR:		// 'or'
    	return OP_AND;
    case RELOP_IN:	// 'in'
    default:
        die("UNEXPECTED_ERROR: op_invert_cond");
        return OP_IGNORE; //keep the compiler happy
    }
}

ir_node_t *eliminate_notop_from_ir_cond(ir_node_t *ir_cond) {
    static int unmatched_not=0;

    if (ir_cond->op_rval==OP_NOT) {
        unmatched_not++;
        ir_cond = eliminate_notop_from_ir_cond(ir_cond->ir_rval2);
        unmatched_not--;
        return ir_cond;
    }

    //eliminate OP_NOT
    if (ir_cond->ir_rval->op_rval==OP_NOT) {
        unmatched_not++;
        ir_cond->ir_rval = eliminate_notop_from_ir_cond(ir_cond->ir_rval->ir_rval2);
        unmatched_not--;
    }

    if (ir_cond->ir_rval2->op_rval==OP_NOT) {
        unmatched_not++;
        ir_cond->ir_rval2 = eliminate_notop_from_ir_cond(ir_cond->ir_rval2->ir_rval2);
        unmatched_not--;
    }

    if (unmatched_not%2==1) {
        switch (ir_cond->op_rval) {
        case RELOP_B:
        case RELOP_BE:
        case RELOP_L:
        case RELOP_LE:
        case RELOP_NE:
        case RELOP_EQU:
            break;
        case OP_AND:
        case OP_OR:
            ir_cond->ir_rval = eliminate_notop_from_ir_cond(ir_cond->ir_rval);
            ir_cond->ir_rval2 = eliminate_notop_from_ir_cond(ir_cond->ir_rval2);
        break;
        case OP_NOT:
            die("UNEXPECTED ERROR: ir_rval_to_ir_cond: not operator still alive");
        default:
            die("UNEXPECTED ERROR: ir_rval_to_ir_cond: bad operator");
        }

        //printf("%s --> %s\n",op_literal(ir_cond->op_rval),op_literal(op_invert_cond(ir_cond->op_rval)));
        ir_cond->op_rval = op_invert_cond(ir_cond->op_rval);
    }

    return ir_cond;
}

ir_node_t *ir_tree_to_ir_cond(ir_node_t *ir_rval) {
    switch (ir_rval->op_rval) {
    case RELOP_B:
    case RELOP_BE:
    case RELOP_L:
    case RELOP_LE:
    case RELOP_NE:
    case RELOP_EQU:
        return ir_rval;
    case OP_AND:
    case OP_OR:
        ir_rval->ir_rval = ir_tree_to_ir_cond(ir_rval->ir_rval);
        ir_rval->ir_rval2 = ir_tree_to_ir_cond(ir_rval->ir_rval2);
        //do not break;
    case OP_NOT:
        ir_rval->ir_rval2 = ir_tree_to_ir_cond(ir_rval->ir_rval2);
        ir_rval->node_type = NODE_BRANCH_COND;
        break;
    default:
        die("UNEXPECTED ERROR: ir_rval_to_ir_cond: bad operator");
    }

    return ir_rval;
}

ir_node_t *expr_tree_to_ir_cond(expr_t *ltree) {
    ir_node_t *ir_cond;

    ir_cond = expr_tree_to_ir_tree(ltree);

    if (ir_cond->data_is!=TYPE_BOOLEAN) {
        die("UNEXPECTED ERROR: expr_tree_to_ir_cond(): not boolean");
    }

    if (ir_cond->op_rval==OP_IGNORE) {
        die("UNEXPECTED ERROR: expr_tree_to_ir_cond(): boolean const or load value with no comparison to zero");
    }

    ir_cond = ir_tree_to_ir_cond(ir_cond);
    ir_cond = eliminate_notop_from_ir_cond(ir_cond);

    return ir_cond;
}

ir_node_t *expr_tree_to_ir_tree(expr_t *ltree) {
    ir_node_t *new_node;
    ir_node_t *convert_node;
    ir_node_t *new_func_call;
    ir_node_t *tmp_node;

    expr_t *tmp_expr;

    sem_t *sem_1;

    //we do not handle EXPR_STRING here, strings can be only in assignments (not inside expressions)

    //the EXPR_LOST is actually a EXPR_LVAL with parsing errors, just load from lost_var

    if (!ltree) {
        //this at the worst case MUST be EXPR_LOST
        die("UNEXPECTED_ERROR: 99-1: NULL expr_tree_to_ir_tree()");
    }

    if (ltree->expr_is==EXPR_RVAL) {
        if (ltree->op==RELOP_IN) {
            //operator 'in' applies only to one set, see expr_distribute_inop_to_set() in expr_toolbox.c
            if (ltree->l2->expr_is==EXPR_LVAL) {
                new_node = make_bitmap_inop_check(ltree);

                //this should go inside the make_inop_check
                new_node->data_is = TYPE_BOOLEAN;

                return new_node;
            }
            else if (ltree->l2->expr_is==EXPR_SET || ltree->l2->expr_is==EXPR_NULL_SET) {
                new_node = make_dynamic_inop_check(ltree);

                //this should go inside make_dynamic_inop_check
                new_node->data_is = TYPE_BOOLEAN;

                return new_node;
            }
            else {
                die("UNEXPECTED_ERROR: 99-2: expr_tree_to_ir_tree()");
            }
        }
        else { //it's all the same
            tmp_node = NULL;

            switch (ltree->op) {
            case RELOP_B:
            case RELOP_BE:
            case RELOP_L:
            case RELOP_LE:
            case RELOP_NE:
            case RELOP_EQU:
                //default is IR_RVAL, if we need only to compare them we must set them as NODE_BRANCH_COND
                break;
            case OP_PLUS:
            case OP_MINUS:
            case OP_MULT:
                tmp_node = ir_move_reg(&R_lo);
                break;
            case OP_RDIV:
                break;
            case OP_DIV:
                tmp_node = ir_move_reg(&R_lo);
                break;
            case OP_MOD:
                tmp_node = ir_move_reg(&R_hi);
                break;
            case OP_AND:
            case OP_OR:
                if (ltree->l1->expr_is==EXPR_LVAL || ltree->l1->expr_is==EXPR_HARDCODED_CONST) {
                    tmp_expr = expr_from_hardcoded_boolean(0);
                    ltree->l1 = expr_relop_equ_addop_mult(ltree->l1,RELOP_NE,tmp_expr);
                }
                //do not break here
            case OP_NOT:
                if (ltree->l2->expr_is==EXPR_LVAL || ltree->l2->expr_is==EXPR_HARDCODED_CONST) {
                    tmp_expr = expr_from_hardcoded_boolean(0);
                    ltree->l2 = expr_relop_equ_addop_mult(ltree->l2,RELOP_NE,tmp_expr);
                }
                break;
            case OP_SIGN:
                //this is a virtual operator, should never reach here
                die("UNEXPECTED_ERROR: expr_tree_to_ir_tree: OP_SIGN");
            default:
                die("UNEXPECTED ERROR: expr_tree_to_ir_tree: inop in RVAL");
            }

            new_node = new_ir_node_t(NODE_RVAL);
            new_node->op_rval = ltree->op;
            new_node->data_is = ltree->datatype->is;

            if (ltree->op!=OP_NOT) {
                //special case for OP_NOT it uses only the l2 expr, see expr_toolbox.c, expressions.c
                new_node->ir_rval = expr_tree_to_ir_tree(ltree->l1);
            }

            new_node->ir_rval2 = expr_tree_to_ir_tree(ltree->l2);

            //this ignores the tmp_node if NULL
            new_node = link_ir_to_ir(tmp_node,new_node);

            if (ltree->convert_to==SEM_INTEGER) {
                convert_node = new_ir_node_t(NODE_CONVERT_TO_INT);
                convert_node->ir_rval = new_node;
                convert_node->data_is = TYPE_INT;
                return convert_node;
            }
            else if (ltree->convert_to==SEM_REAL) {
                convert_node = new_ir_node_t(NODE_CONVERT_TO_REAL);
                convert_node->ir_rval = new_node;
                convert_node->data_is = TYPE_REAL;
                return convert_node;
            }
            return new_node;
        }
    }
    else if (ltree->expr_is==EXPR_HARDCODED_CONST) {
        if (ltree->ival==0 || ltree->fval==0 || ltree->cval==0) {
            new_node = new_ir_node_t(NODE_RVAL_ARCH);
            new_node->reg = &R_zero;
        } else {
            new_node = new_ir_node_t(NODE_HARDCODED_RVAL);
            new_node->ival = ltree->ival;
            new_node->fval = ltree->fval;
            new_node->cval = ltree->cval;
            new_node->data_is = ltree->datatype->is;
        }
        return new_node;
    }
    else if (ltree->expr_is==EXPR_NULL_SET) {
        new_node = new_ir_node_t(NODE_INIT_NULL_SET);
        new_node->ir_lval_dest = calculate_lvalue(ltree->var);
        new_node->data_is = TYPE_INT;
        return new_node;
    }
    else if (ltree->expr_is==EXPR_SET) {
        //convert to bitmap, this set goes for assignment
        //we enter here with an assign statement like:
        //"set_variable := [5,10..15,20] + lval_set * another_lval_set - [7,8]
        new_node = create_bitmap(ltree);
        new_node->data_is = TYPE_SET;
        return new_node;
    }
    else if (ltree->expr_is==EXPR_LVAL || ltree->expr_is==EXPR_LOST) {
        //if (ltree->datatype->is==TYPE_SET) {
	//    //we handle differently the lvalue sets, see new_assignment() in ir.c
        //    die("UNEXPECTED_ERROR: 99-3: EXPR_LVAL of TYPE_SET in expr_tree_to_ir_tree()");
        //}

        //load the lvalue of every datatype

        new_node = new_ir_node_t(NODE_LOAD);
        new_node->address = calculate_lvalue(ltree->var);

        if (ltree->var->id_is==ID_RETURN) {
            sem_1 = sm_find(ltree->var->name);
            new_func_call = prepare_stack_and_call(sem_1->subprogram,ltree->expr_list);

            //finally we must read the return value after the actual call
            new_node = link_ir_to_ir(new_node,new_func_call);
        }

        new_node->data_is = ltree->datatype->is;

        if (ltree->convert_to==SEM_INTEGER) {
            convert_node = new_ir_node_t(NODE_CONVERT_TO_INT);
            convert_node->ir_rval = new_node;
            convert_node->data_is = TYPE_INT;
            return convert_node;
        }
        else if (ltree->convert_to==SEM_REAL) {
            convert_node = new_ir_node_t(NODE_CONVERT_TO_REAL);
            convert_node->ir_rval = new_node;
            convert_node->data_is = TYPE_REAL;
            return convert_node;
        }
        return new_node;
    }

    die("UNEXPECTED_ERROR: 99-5: UNKNOWN EXPR type in expr_tree_to_ir_tree()");
    return NULL; //keep the compiler happy
}

ir_node_t *calculate_lvalue(var_t *v) {
    ir_node_t *new_node;
    ir_node_t *ptr_deref;

    expr_t *address_expr;

    if (!v) {
        die("UNEXPECTED_ERROR: 74-1");
    }

    //calculate the reference address
    if (v->Lvalue->content_type==PASS_REF) {
        new_node = new_ir_node_t(NODE_HARDCODED_LVAL);
        new_node->lval = v->Lvalue;
        new_node->ival = v->Lvalue->seg_offset->ival;

        //set the address so far
        new_node->address = expr_tree_to_ir_tree(v->Lvalue->seg_offset);

        //load the final address
        ptr_deref = new_ir_node_t(NODE_LOAD);
        ptr_deref->address = new_node;

        //put the offset separate after the load
        new_node = new_ir_node_t(NODE_LVAL);
        new_node->address = ptr_deref;
        new_node->offset = expr_tree_to_ir_tree(v->Lvalue->offset_expr);
    } else { //PASS_VAL
        if (v->Lvalue->offset_expr->expr_is==EXPR_HARDCODED_CONST) {
            new_node = new_ir_node_t(NODE_HARDCODED_LVAL);
            new_node->lval = v->Lvalue;
            new_node->ival = v->Lvalue->seg_offset->ival + v->Lvalue->offset_expr->ival;

            //put the offset together with the *address
            address_expr = expr_relop_equ_addop_mult(v->Lvalue->seg_offset,OP_PLUS,v->Lvalue->offset_expr);
            new_node->address = expr_tree_to_ir_tree(address_expr);
        } else {
            new_node = new_ir_node_t(NODE_LVAL);
            new_node->address = expr_tree_to_ir_tree(v->Lvalue->seg_offset);
            new_node->offset = expr_tree_to_ir_tree(v->Lvalue->offset_expr);
        }
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

ir_node_t *prepare_stack_and_call(func_t *subprogram, expr_list_t *list) {
    ir_node_t *new_stack_init_node;
    ir_node_t *ir_jump_link;
    ir_node_t *tmp_assign;

    data_t *el_datatype;
    var_t *tmp_var;

    int i;

    if (!list && subprogram->param_num!=0) {
        sprintf(str_err,"'%s' subprogram takes %d parameters",subprogram->func_name,subprogram->param_num);
        yyerror(str_err);
        return new_lost_ir_node("__BAD_STACK_PREPARATION__");
    }

    if (list->all_expr_num != subprogram->param_num) {
        sprintf(str_err,"invalid number of parameters: subprogram `%s` takes %d parameters",subprogram->func_name,subprogram->param_num);
        yyerror(str_err);
        return new_lost_ir_node("__BAD_STACK_PREPARATION__");
    }

    if (MAX_EXPR_LIST - list->expr_list_empty == subprogram->param_num) {
        new_stack_init_node = NULL;

        tmp_var = (var_t*)malloc(sizeof(var_t)); //allocate once
        tmp_var->id_is = ID_VAR;
#warning do we need the scope of the callee here?
        tmp_var->scope = get_current_scope();
        tmp_var->cond_assign = NULL;

        for (i=0;i<subprogram->param_num;i++) {
            tmp_assign = NULL;

            //continue ir generation
            //if (list->expr_list[i]->expr_is==EXPR_LOST) {
            //    //parse errors
            //    free(tmp_var);
            //    return NULL;
            //}

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

                    return new_lost_ir_node("__BAD_STACK_PREPARATION__");
                }

                if (el_datatype != subprogram->param[i]->datatype) {
                    sprintf(str_err,"passing refference to datatype '%s' with datatype '%s'",subprogram->param[i]->datatype->data_name,el_datatype->data_name);
                    yyerror(str_err);
                    free(tmp_var);

                    return new_lost_ir_node("__BAD_STACK_PREPARATION__");
                }

                if (list->expr_list[i]->var->id_is==ID_VAR_GUARDED) {
                    sprintf(str_err,"guard variable of for_statement '%s' passed by refference",list->expr_list[i]->var->name);
                    yyerror(str_err);
                    free(tmp_var);

                    return new_lost_ir_node("__BAD_STACK_PREPARATION__");
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

                //convert to RVAL
#warning should I mark the ir_rval as NODE_RVAL here?
                tmp_assign->ir_rval->node_type = NODE_RVAL;
                tmp_assign->ir_rval->ir_rval = tmp_assign->ir_rval->address;
                if (tmp_assign->ir_rval->node_type==NODE_LVAL) {
                    tmp_assign->ir_rval->ir_rval2 = tmp_assign->ir_rval->offset;
                }

                //restore the PASS_REF content_type
                tmp_var->Lvalue->content_type = PASS_REF;
            }
            else { //PASS_VAL
                //accept anything, if it's not rvalue we pass it's address
                if (TYPE_IS_COMPOSITE(el_datatype)) {
                    yyerror("arrays, records and set datatypes can only be passed by refference");
                    free(tmp_var);

                    return new_lost_ir_node("__BAD_STACK_PREPARATION__");
                }
                tmp_assign = new_ir_assign(tmp_var,list->expr_list[i]);
            }
#warning "if errors, we leak memory"

            new_stack_init_node = link_ir_to_ir(tmp_assign,new_stack_init_node);
        }
        free(tmp_var);

        ir_jump_link = jump_and_link_to(subprogram);
        new_stack_init_node = link_ir_to_ir(ir_jump_link,new_stack_init_node);

        return new_stack_init_node;
    }

    //some expressions are invalid, but their number is correct, avoid some unreal error messages afterwards
    return new_lost_ir_node("__BAD_STACK_PREPARATION__");
}

