#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "semantics.h"
#include "symbol_table.h"
#include "datatypes.h"
#include "ir.h"
#include "ir_toolbox.h"
#include "bitmap.h"
#include "expressions.h"
#include "expr_toolbox.h"
#include "err_buff.h"
#include "reg.h"

int pow2(int x) {
    int pow = 0;

    if (! (x & (x - 1)))
        while (x > 1) {
            pow++;
            x /= 2;
        }
    return pow;
}

ir_node_t *optimize_math(ir_node_t *node) {
    //recursive function, some children may not exist (e.g. OP_NOT has no ir_rval child)
    if (!node)
        return NULL;

    if (node->node_type != NODE_RVAL)
        return node;

    node->ir_rval = optimize_math(node->ir_rval);
    node->ir_rval2 = optimize_math(node->ir_rval2);

    if (node->ir_rval &&
        node->ir_rval->node_type == NODE_HARDCODED_RVAL &&
        node->ir_rval2->node_type != NODE_HARDCODED_RVAL) {
        //swap nodes
        ir_node_t *tmp = node->ir_rval;
        node->ir_rval = node->ir_rval2;
        node->ir_rval2 = tmp;
    }

    if (node->ir_rval &&
        node->ir_rval->node_type != NODE_HARDCODED_RVAL &&
        node->ir_rval2->node_type == NODE_HARDCODED_RVAL) {

        int ival = node->ir_rval2->ival;
        int log_ival = pow2(ival);

        //ignore optimization if ival is not power of 2
        if (!log_ival)
            return node;

        switch (node->op_rval) {
        case OP_MULT:
            node->node_type = NODE_SHIFT_LEFT;
            node->ir_rval2->ival = log_ival;
            //printf("debug: mult converted to shift: log(%d) = %d\n",ival,log_ival);
            break;
        case OP_DIV:
            node->node_type = NODE_SHIFT_RIGHT;
            node->ir_rval2->ival = log_ival;
            //printf("debug: div converted to shift: log(%d) = %d\n",ival,log_ival);
            break;
        default:
            break;
        }
    }

    return node;
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
            die("UNEXPECTED ERROR: eliminate_notop_from_ir_cond: not operator still alive");
        default:
            die("UNEXPECTED ERROR: eliminate_notop_from_ir_cond: bad operator");
        }

        //printf("%s --> %s\n",op_literal(ir_cond->op_rval),op_literal(op_invert_cond(ir_cond->op_rval)));
        ir_cond->op_rval = op_invert_cond(ir_cond->op_rval);
    }

    return ir_cond;
}

ir_node_t *ir_tree_to_ir_cond(ir_node_t *ir_rval) {
    if (ir_rval->op_rval==OP_IGNORE) {
        die("UNEXPECTED ERROR: ir_tree_to_ir_cond(): boolean const or load value with no comparison to zero");
    }

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
        //do not break;
    case OP_NOT:
        ir_rval->ir_rval2 = ir_tree_to_ir_cond(ir_rval->ir_rval2);
        ir_rval->node_type = NODE_BRANCH;
        break;
    default:
        //printf("debug: operator %s\t",op_literal(ir_rval->op_rval));
        die("UNEXPECTED ERROR: ir_tree_to_ir_cond: bad operator");
    }

    return ir_rval;
}

ir_node_t *expr_tree_to_ir_cond(expr_t *ltree) {
    ir_node_t *ir_cond;

    if (ltree->datatype->is!=TYPE_BOOLEAN)
        die("UNEXPECTED ERROR: expr_tree_to_ir_cond(): not boolean");

    ir_cond = expr_tree_to_ir_tree(ltree);
    ir_cond = ir_tree_to_ir_cond(ir_cond);
    ir_cond = eliminate_notop_from_ir_cond(ir_cond);

    return ir_cond;
}

ir_node_t *expr_tree_to_ir_tree(expr_t *ltree) {
    ir_node_t *new_node;
    ir_node_t *convert_node;
    ir_node_t *new_func_call;

    expr_t *tmp_expr;

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
            switch (ltree->op) {
            case RELOP_B:
            case RELOP_BE:
            case RELOP_L:
            case RELOP_LE:
            case RELOP_NE:
            case RELOP_EQU:
                //default is IR_RVAL, if we need only to compare them we must set them as NODE_BRANCH
                break;
            case OP_PLUS:
            case OP_MINUS:
            case OP_MULT:
            case OP_RDIV:
            case OP_DIV:
            case OP_MOD:
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

            new_node = optimize_math(new_node);

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
        //if (ltree->ival==0 || ltree->fval==0 || ltree->cval==0) {
        //    new_node = new_ir_node_t(NODE_RVAL);
        //    new_node->reg = &R_zero;
        //} else {
        new_node = new_ir_node_t(NODE_HARDCODED_RVAL);
        new_node->ival = ltree->ival;
        new_node->fval = ltree->fval;
        new_node->cval = ltree->cval;
        new_node->data_is = ltree->datatype->is;
            //}
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

        if (ltree->var->id_is==ID_RETURN) {
            if (ltree->var->scope->status==FUNC_OBSOLETE) {
                die("UNEXPECTED_ERROR: ir_toolbox.c: obsolete function still has lvalue?");

                /*
                new_node = new_ir_node_t(NODE_HARDCODED_RVAL);
                new_node->ival = ltree->var->ival;
                new_node->fval = ltree->var->fval;
                new_node->cval = ltree->var->cval;
                new_node->data_is = ltree->var->datatype->is;

                return new_node;
                */
            }

            //printf("debug: function call: %s\treturn_value: %s\n", ltree->var->scope->name, ltree->var->name);
            new_func_call = prepare_stack_and_call(ltree->var->scope,ltree->expr_list);

            //finally we must read the return value after the actual call
            new_node = new_ir_node_t(NODE_LOAD);
            new_node->ir_lval = calculate_lvalue(ltree->var);
            new_node->data_is = ltree->datatype->is;

            new_node = link_ir_to_ir(new_node,new_func_call);

        } else {
            ir_node_t *tmp = calculate_lvalue(ltree->var);
            /*
            if (VAR_LIVES_IN_REGISTER(ltree->var))
                return tmp;
            */
            new_node = new_ir_node_t(NODE_LOAD);
            new_node->ir_lval = tmp;
            new_node->data_is = ltree->datatype->is;
            //new_node->lval_name = ltree->var->name;
        }

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

inline void consider_all_offsets(var_t *var) {
    expr_t *relative_offset;
    expr_t *final_offset;
    expr_t *cond;
    expr_t *final_cond;

    mem_t *new_mem;
    mem_t *base_Lvalue;

    info_comp_t *comp;
    var_t *base;
    expr_list_t *index;

    if (var->from_comp && var->Lvalue)
        die("INTERNAL_ERROR: ir_toolbox.c: Lvalue already?");

    if (var->Lvalue)
        return;

    //we must create the final Lvalue
    final_offset = expr_from_hardcoded_int(0);
    final_cond = expr_from_hardcoded_boolean(1);

    comp = var->from_comp;
    while (comp) {
        switch (comp->comp_type) {
        case TYPE_ARRAY:
            base = comp->array.base;
            index = comp->array.index;
            base_Lvalue = comp->array.base->Lvalue;

            if (!valid_expr_list_for_array_reference(comp)) {
                //static bound checks failed in IR level
                //the array reference was valid in the frontend, see datatypes.c: reference_to_array_element()
                //the loop optimizer has bugs
                die("INTERNAL_ERROR: we generate out of bounds array references");
            }

            relative_offset = make_array_reference(comp);
            cond = make_array_bound_check(comp);
            final_cond = expr_orop_andop_notop(final_cond,OP_AND,cond);

            break;
        case TYPE_RECORD:
            base = comp->record.base;
            base_Lvalue = comp->record.base->Lvalue;
            relative_offset = comp->record.el_offset;
            break;
        default:
            die("UNEXPECTED_ERROR: no array/record in variable_from_comp_datatype_element()");
        }

        final_offset = expr_relop_equ_addop_mult(final_offset,OP_PLUS,relative_offset);

        comp = base->from_comp;
    }

#warning enable me
    //var->cond_assign = final_cond;

    new_mem = (mem_t*)calloc(1,sizeof(mem_t));
    new_mem = (mem_t*)memcpy(new_mem,base_Lvalue,sizeof(mem_t));
    new_mem->offset_expr = final_offset;
    new_mem->size = var->datatype->memsize;

    var->Lvalue = new_mem;

    return;
}

ir_node_t *calculate_lvalue(var_t *v) {
    ir_node_t *new_node;
    ir_node_t *base_address;

    expr_t *offset_expr;

    if (!v)
        die("UNEXPECTED_ERROR: 74-1");

    if (VAR_LIVES_IN_REGISTER(v)) {
        new_node = new_ir_node_t(NODE_RVAL_ARCH);

        if (v->Lvalue->reg) {
            //already given a register, use it
            new_node->reg = v->Lvalue->reg;
            //printf("debug: we meeet again!\n");
        }
        else {
            //first reference of variable, get a virtual register
            v->Lvalue->reg = new_node->reg;
            //printf("debug: first reference of register only variable\n");
        }

        return new_node;
    }

    //variable lives in memory, proceed as usual
    consider_all_offsets(v);

    if (v->Lvalue->segment==MEM_GLOBAL) {
#if (USE_PSEUDO_INSTR_LA==1)
        base_address = new_ir_node_t(NODE_LVAL_NAME);
        base_address->lval_name = v->name;
#else
        base_address = new_ir_node_t(NODE_RVAL_ARCH);
        base_address->reg = &R_gp;
#endif
    } else {
        base_address = new_ir_node_t(NODE_RVAL_ARCH);
        base_address->reg = &R_sp;
    }

    //calculate the reference address
    if (v->Lvalue->content_type==PASS_REF) {
        new_node = new_ir_node_t(NODE_LVAL);

        //set the address so far
        new_node->address = base_address;

#if (USE_PSEUDO_INSTR_LA==1)
        if (v->Lvalue->segment==MEM_GLOBAL)
            new_node->offset = expr_tree_to_ir_tree(expr_from_hardcoded_int(0));
        else
#endif
            new_node->offset = expr_tree_to_ir_tree(v->Lvalue->seg_offset);

        //load the final address
        base_address = new_ir_node_t(NODE_LOAD);
        base_address->ir_lval = new_node;

        offset_expr = expr_from_hardcoded_int(0);
    } else {
#if (USE_PSEUDO_INSTR_LA==1)
        if (v->Lvalue->segment==MEM_GLOBAL)
            offset_expr = expr_from_hardcoded_int(0);
        else
#endif
            offset_expr = v->Lvalue->seg_offset;
    }

    if (v->Lvalue->offset_expr->expr_is==EXPR_HARDCODED_CONST) {
        new_node = new_ir_node_t(NODE_LVAL);

        offset_expr = expr_relop_equ_addop_mult(offset_expr,OP_PLUS,v->Lvalue->offset_expr);

        new_node->address = base_address;
        new_node->offset = expr_tree_to_ir_tree(offset_expr);
    } else {
        new_node = base_address;
        base_address = new_ir_node_t(NODE_RVAL);
        base_address->op_rval = OP_PLUS;
        base_address->ir_rval = new_node;
        base_address->ir_rval2 = expr_tree_to_ir_tree(v->Lvalue->offset_expr);

        new_node = new_ir_node_t(NODE_LVAL);
        new_node->address = base_address;
        new_node->offset = expr_tree_to_ir_tree(offset_expr);
    }

    new_node->data_is = v->datatype->is;

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

    var_t *tmp_var;

    int i;

    //we are error free here!
    new_stack_init_node = NULL;

    tmp_var = (var_t*)calloc(1,sizeof(var_t)); //allocate once
    tmp_var->id_is = ID_VAR;
    //#warning do we need the scope of the callee here?
    tmp_var->scope = NULL;
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

        if (subprogram->param[i]->pass_mode==PASS_REF) {
            //mark temporarily the lvalue as PASS_VAL, in order for the calculate_lvalue() to work properly
            //this is a very ugly way of doing things :( //FIXME
            tmp_var->Lvalue->content_type = PASS_VAL;

            //reminder: we are talking about stack Lvalues
            //parameters do not change position in stack

            tmp_assign = new_ir_node_t(NODE_ASSIGN);
            tmp_assign->ir_lval = calculate_lvalue(tmp_var);
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
            tmp_assign = new_ir_assign(tmp_var,list->expr_list[i]);
        }

        new_stack_init_node = link_ir_to_ir(tmp_assign,new_stack_init_node);
    }
    free(tmp_var);

    ir_jump_link = jump_and_link_to(subprogram);
    new_stack_init_node = link_ir_to_ir(ir_jump_link,new_stack_init_node);

    return new_stack_init_node;
}
