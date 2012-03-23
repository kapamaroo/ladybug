#include <stdlib.h>
#include <string.h> //memcpy()

#include "build_flags.h"
#include "semantics.h"
//#include "scope.h"
#include "symbol_table.h"
#include "datatypes.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "mem.h"
#include "ir.h"
#include "ir_toolbox.h"
#include "bitmap.h"
#include "err_buff.h"

unsigned long unique_virt_reg = 40;

ir_node_t *ir_root_tree[MAX_NUM_OF_MODULES];

ir_node_t *new_ir_assign_str(var_t *v, expr_t *l);
ir_node_t *new_ir_assign_expr(var_t *v, expr_t *l);
ir_node_t *expand_array_assign(var_t *v,expr_t *l);
ir_node_t *expand_record_assign(var_t *v,expr_t *l);
ir_node_t *backpatch_ir_cond(ir_node_t *ir_cond,ir_node_t *ir_true,ir_node_t *ir_false);
var_t *variable_from_comp_datatype_element(var_t *var);

reg_t *new_virtual_register() {
    reg_t *new_reg;

    new_reg = (reg_t*)malloc(sizeof(reg_t));
    new_reg->type = REG_VIRT;
    new_reg->virtual = unique_virt_reg++;
    new_reg->live = NULL;
    new_reg->die = NULL;
    new_reg->physical = NULL;

    return new_reg;
}

char *new_label_literal(char *label) {
    return strdup(label);
}

char *new_label_unique(char *prefix) {
    static int n = 0;
    char label_buf[MAX_LABEL_SIZE];

    snprintf(label_buf,MAX_LABEL_SIZE,"L_%s_%d",prefix,n);
    n++;
    return strdup(label_buf);
}

char *new_label_subprogram(char *sub_name) {
    //char label_buf[MAX_LABEL_SIZE];

    //snprintf(label_buf,MAX_LABEL_SIZE,"S_%s",sub_name);
    //return strdup(label_buf);
    return strdup(sub_name);
}

void init_ir() {
    int i;

    //first of all initialize the register counter

    for(i=0; i<MAX_NUM_OF_MODULES; i++) {
        ir_root_tree[i] = NULL;
    }

    ir_root_tree[0] = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_root_tree[0]->label = new_label_literal("main");

    init_bitmap();
}

void new_ir_tree(func_t *subprogram) {
    ir_node_t *ir_new;

    ir_new = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_new->label = new_label_subprogram(subprogram->name);

    ir_root_tree[subprogram->unique_id] = ir_new;
}

ir_node_t *link_ir_to_ir(ir_node_t *child,ir_node_t *parent) {
    //return the head of the linked list
    if (parent && child) {
        parent->last->next = child;
        child->prev = parent->last;
        parent->last = child->last;
        return parent;
    } else if (!parent) {
        return child;
    } else {
        return parent;
    }
}

ir_node_t *new_ir_node_t(ir_node_type_t node_type) {
    ir_node_t *new_node;

    new_node = (ir_node_t*)malloc(sizeof(ir_node_t));
    new_node->node_type = node_type;

    new_node->reg = new_virtual_register();
    new_node->op_rval = OP_IGNORE;

    new_node->ir_lval = NULL;
    new_node->ir_lval2 = NULL;
    new_node->ir_rval = NULL;
    new_node->ir_rval2 = NULL;

    new_node->address = NULL;
    new_node->offset = NULL;
    new_node->ir_lval_dest = NULL;
    new_node->ir_goto = NULL;

    new_node->next = NULL;
    new_node->prev = NULL; //new_node;
    new_node->last = new_node;
    new_node->label = NULL;
    return new_node;
}

ir_node_t *new_ir_assign(var_t *v, expr_t *l) {
    ir_node_t *new_stmt;

    if (TYPE_IS_STRING(v->datatype)) {
        new_stmt = new_ir_assign_str(v,l);
    } else {
        new_stmt = new_ir_assign_expr(v,l);
    }

    return new_stmt;
}

ir_node_t *new_ir_assign_str(var_t *v, expr_t *l) {
    ir_node_t *new_stmt;

    v = variable_from_comp_datatype_element(v);

    //strings terminate with 0, copy until array is full
    new_stmt = new_ir_node_t(NODE_ASSIGN_STRING);
    new_stmt->ir_lval = calculate_lvalue(v);
    new_stmt->ir_lval2 = calculate_lvalue(l->var);
    return new_stmt;
}

ir_node_t *new_ir_assign_expr(var_t *v, expr_t *l) {
    ir_node_t *new_stmt;
    ir_node_t *tmp1;
    ir_node_t *tmp2;
    expr_t *expr1;
    expr_t *expr2;

    switch (v->datatype->is) {
    case TYPE_INT:
        //standard types are scalar but real
        if (l->datatype->is==TYPE_REAL) {
	    l->convert_to = SEM_INTEGER;
        }
        break;
    case TYPE_REAL:
        if (l->datatype->is!=TYPE_REAL) {
	    l->convert_to = SEM_REAL;
        }
        break;
    case TYPE_BOOLEAN:
        //we must assign either 1 or 0
        //convert statement to branch
        //do NOT even think about recursion in this case! :)

        //true
        expr1 = expr_from_hardcoded_boolean(1);
        tmp1 = new_ir_node_t(NODE_ASSIGN);
        tmp1->ir_lval = calculate_lvalue(v);
        tmp1->ir_rval = expr_tree_to_ir_tree(expr1);

        //false
        expr2 = expr_from_hardcoded_boolean(0);
        tmp2 = new_ir_node_t(NODE_ASSIGN);
        tmp2->ir_lval = tmp1->address;
        tmp2->ir_rval = expr_tree_to_ir_tree(expr2);

        new_stmt = new_ir_if(l,tmp1,tmp2);
        return new_stmt;
    case TYPE_CHAR:
        //if expr is char we suppose it has a valid value
        if (l->datatype->is!=TYPE_CHAR) {
            //#warning INFO: allow the next only if we can assign scalar types to chars
            v->cond_assign = make_ASCII_bound_checks(v,l);
        }
        break;
    case TYPE_ENUM:
        //enumerations are integers
        //assign for enumerations
        //#warning INFO: allow the next only if we can assign scalar types to enumerations
        v->cond_assign = make_enum_subset_bound_checks(v,l);
        break;
    case TYPE_SUBSET:
        v->cond_assign = make_enum_subset_bound_checks(v,l);
        break;
    case TYPE_ARRAY:
        if (v->datatype==l->datatype) {
	    //explicit datatype match, optimize: use memcopy
	    //reminder: we do not allow signed arrays in expressions, see expr_sign() in expressions.c
	    new_stmt = new_ir_node_t(NODE_MEMCPY);
	    new_stmt->address = calculate_lvalue(v);
	    new_stmt->ir_lval = calculate_lvalue(l->var);
        } else {
            //compatible datatypes, but not identical (e.g. integer and real)
	    new_stmt = expand_array_assign(v,l);
        }
        return new_stmt;
    case TYPE_RECORD:
        if (v->datatype==l->datatype) {
	    //explicit datatype match, optimize: use memcopy
	    //reminder: we do not allow signed arrays in expressions, see expr_sign() in expressions.c
	    new_stmt = new_ir_node_t(NODE_MEMCPY);
	    new_stmt->address = calculate_lvalue(v);
	    new_stmt->ir_lval = calculate_lvalue(l->var);
        } else {
            //compatible datatypes, but not identical (e.g. integer and real)
	    new_stmt = expand_record_assign(v,l);
        }
        return new_stmt;
    case TYPE_SET:
        new_stmt = new_ir_node_t(NODE_ASSIGN_SET);
        new_stmt->ir_lval = calculate_lvalue(v);
        new_stmt->ir_lval2 = create_bitmap(l);
        return new_stmt;
    case TYPE_VOID: //keep the compiler happy
        die("UNEXPECTED ERROR: TYPE_VOID in assignment");
    }

    /**** some common assign actions */
    v = variable_from_comp_datatype_element(v);

    new_stmt = new_ir_node_t(NODE_ASSIGN);
    new_stmt->ir_lval = calculate_lvalue(v);
    new_stmt->ir_rval = expr_tree_to_ir_tree(l);

    if (v->cond_assign) {
        //we must do some checks before the assignment, convert to branch node
        new_stmt = new_ir_if(v->cond_assign,new_stmt,NULL);
    }

    return new_stmt;
}

ir_node_t *new_ir_if(expr_t *cond,ir_node_t *true_stmt,ir_node_t *false_stmt) {
    ir_node_t *jump_exit_branch;
    ir_node_t *ir_cond;
    ir_node_t *ir_exit_if;

    //true_stmt always exists

    ir_exit_if = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_exit_if->label = new_label_unique("IF_EXIT");
    true_stmt = link_ir_to_ir(ir_exit_if,true_stmt);

    if (false_stmt) {
        /* pseudo assembly
           if cond goto: TRUE_STMT
           FALSE_STMT
           goto: EXIT_LABEL
           TRUE_STMT
           EXIT_LABEL
        */

	jump_exit_branch = jump_to(ir_exit_if);
        false_stmt = link_ir_to_ir(jump_exit_branch,false_stmt);

        //if_node->ir_cond = expr_tree_to_ir_tree(cond);
        ir_cond = expr_tree_to_ir_cond(cond);
        ir_cond = backpatch_ir_cond(ir_cond,true_stmt,false_stmt);
        ir_cond = link_ir_to_ir(false_stmt,ir_cond);
    } else {
        // only true_stmt
        /* pseudo assembly
           if !cond goto: EXIT_LABEL
           TRUE_STMT
           EXIT_LABEL
        */

        cond = expr_orop_andop_notop(NULL,OP_NOT,cond);

        ir_cond = expr_tree_to_ir_cond(cond);
        ir_cond = backpatch_ir_cond(ir_cond,ir_exit_if,true_stmt);
    }

    ir_cond = link_ir_to_ir(true_stmt,ir_cond);

    return ir_cond;
}

ir_node_t *new_ir_while(expr_t *cond,ir_node_t *true_stmt) {
    ir_node_t *while_node;
    ir_node_t *jump_loop_branch;

    /* pseudo assembly
       LABEL_ENTER
       new_ir_if(cond,true_stmt);
     */

    jump_loop_branch = jump_to(NULL); //needs backpatch
    true_stmt = link_ir_to_ir(jump_loop_branch,true_stmt);

    while_node = new_ir_if(cond,true_stmt,NULL);
    while_node->label = new_label_unique("WHILE_ENTER");

    jump_loop_branch->ir_goto = while_node; //do the backpatch

    return while_node;
}

ir_node_t *new_ir_for(var_t *var,iter_t *range,ir_node_t *true_stmt) {
    ir_node_t *dark_init_for;
    ir_node_t *dark_cond_step; //append this to true_stmt
    ir_node_t *for_node;

    expr_t *expr_guard;
    expr_t *expr_step;
    expr_t *left_cond;
    expr_t *right_cond;
    expr_t *total_cond;

    /* pseudo assembly
       INIT_FOR
       new_ir_while(total_cond,true_stmt);
     */

    expr_guard = expr_from_variable(var);
    expr_step = expr_relop_equ_addop_mult(expr_guard,OP_PLUS,range->step);

    left_cond = expr_relop_equ_addop_mult(range->start,RELOP_LE,expr_guard);
    right_cond = expr_relop_equ_addop_mult(expr_guard,RELOP_LE,range->stop);
    total_cond = expr_orop_andop_notop(left_cond,OP_AND,right_cond);

    //ignore any high level info
    dark_init_for = new_ir_assign(var,range->start);
    dark_cond_step = new_ir_assign(var,expr_step);

    dark_init_for->label = new_label_unique("FOR_ENTER");

    true_stmt = link_ir_to_ir(dark_cond_step,true_stmt);

    for_node = new_ir_while(total_cond,true_stmt);
    for_node = link_ir_to_ir(for_node,dark_init_for);

    return for_node;
}

ir_node_t *new_ir_with(ir_node_t *body) {
    //if the variable is a record return the body of the statement
    //the body is a comp_statement
    return body;
}

ir_node_t *new_ir_procedure_call(func_t *subprogram,expr_list_t *list) {
    ir_node_t *new_proc_call;

    //we are error free here!
    //printf("debug: jump to: %s\n", subprogram->name);
    new_proc_call = prepare_stack_and_call(subprogram,list);
    return new_proc_call;
}

ir_node_t *new_ir_comp_stmt(ir_node_t *body) {
    return body;
}

/** Assume that read and write statements have meaning only for standard types and STRINGS
 * these are the only serializeable datatypes
 * booleans are considered chars here and we must check their value after we read them
 * chars are considered STRINGS of size 1
 */
ir_node_t *new_ir_read(var_list_t *list) {
    int i;
    ir_node_t *new_ir;
    ir_node_t *read_stmt;
    ir_node_t *tmp;
    ir_node_t *full_addr;

    var_t *v;
    data_t *d;

    //we are error free here!
    read_stmt = NULL;
    for(i=0;i<MAX_VAR_LIST-list->var_list_empty;i++) {
        v = variable_from_comp_datatype_element(list->var_list[i]);
        d = v->datatype;

        new_ir = new_ir_node_t(NODE_SYSCALL);
        full_addr = calculate_lvalue(v);

        switch (v->id_is) {
        case ID_VAR:
            new_ir->address = full_addr;

            if (d->is==TYPE_INT) {
                new_ir->syscall_num = SVC_READ_INT;
            }
            else if (d->is==TYPE_REAL) {
                new_ir->syscall_num = SVC_READ_REAL; //single float
            }
            else if (d->is==TYPE_BOOLEAN || d->is==TYPE_CHAR) {
                //read char and check the value
                new_ir->syscall_num = SVC_READ_CHAR;
            }
            else if (TYPE_IS_STRING(d)) {
                new_ir->syscall_num = SVC_READ_STRING;

                tmp = new_ir_node_t(NODE_RVAL);
                tmp->op_rval = OP_PLUS;
                tmp->ir_rval = full_addr->address;
                tmp->ir_rval2 = full_addr->offset;

                new_ir->ir_rval = tmp;
                new_ir->ival = v->datatype->dim[0]->range;  //string size
            } else {
                die("UNEXPECTED_ERROR: 44-42");
            }

            new_ir = link_ir_to_ir(new_ir,read_stmt);
            break;
        default:
            die("UNEXPECTED_ERROR: 44-44");
        }
    }
    return read_stmt;
}

ir_node_t *new_ir_write(expr_list_t *list) {
    int i;
    ir_node_t *new_ir;
    ir_node_t *write_stmt;
    ir_node_t *tmp;
    ir_node_t *full_addr;
    expr_t *l;
    data_t *d;

    //we are error free here!
    write_stmt = NULL;
    for(i=0;i<MAX_EXPR_LIST-list->expr_list_empty;i++) {
        l = list->expr_list[i];

        new_ir = new_ir_node_t(NODE_SYSCALL);

        if (l->expr_is==EXPR_STRING) {
            new_ir->syscall_num = SVC_PRINT_STRING;
            full_addr = calculate_lvalue(l->var);

            tmp = new_ir_node_t(NODE_RVAL);
            tmp->op_rval = OP_PLUS;
            tmp->ir_rval = full_addr->address;
            tmp->ir_rval2 = full_addr->offset;

            new_ir->ir_rval = tmp;
        }
        else if (l->expr_is==EXPR_RVAL || l->expr_is==EXPR_HARDCODED_CONST || l->expr_is==EXPR_LVAL) {
            d = l->datatype;

            new_ir->ir_rval = expr_tree_to_ir_tree(l);

            if (d->is==TYPE_INT) {
                new_ir->syscall_num = SVC_PRINT_INT;
            }
            else if (d->is==TYPE_REAL) {
                new_ir->syscall_num = SVC_PRINT_REAL; //single float
            }
            else if (d->is==TYPE_BOOLEAN) {
                //print as integer
                new_ir->syscall_num = SVC_PRINT_CHAR;
            }
            else if (d->is==TYPE_CHAR) {
                new_ir->syscall_num = SVC_PRINT_CHAR;
            }
            else {
                die("UNEXPECTED_ERROR: 44-46");
            }
        }
        else {
            die("UNEXPECTED_ERROR: 44-45");
        }
        new_ir = link_ir_to_ir(new_ir,write_stmt);
    }
    return write_stmt;
}

ir_node_t *jump_and_link_to(func_t *subprogram) {
    ir_node_t *new_jump_link;

    new_jump_link = new_ir_node_t(NODE_JUMP_LINK);
    new_jump_link->ir_goto = ir_root_tree[subprogram->unique_id];
    return new_jump_link;
}

ir_node_t *jump_to(ir_node_t *ir_dest) {
    ir_node_t *new_jump_link;

    new_jump_link = new_ir_node_t(NODE_JUMP);
    new_jump_link->ir_goto = ir_dest;
    return new_jump_link;
}

ir_node_t *expand_array_assign(var_t *v,expr_t *l) {
    //expression is EXPR_LVAL here
    var_t *dummy_var_array;
    mem_t *dummy_mem_array;

    var_t *dummy_var_l;
    mem_t *dummy_mem_l;

    expr_t *dummy_expr;

    int i;
    int elem_offset_num;

    ir_node_t *new_stmt;

    elem_offset_num = v->datatype->dim[0]->relative_distance*v->datatype->dim[0]->range*v->datatype->def_datatype->memsize;
    new_stmt = NULL;
    for(i=0; i<elem_offset_num; i+=v->datatype->def_datatype->memsize) {
        dummy_mem_array = (mem_t*)malloc(sizeof(mem_t));
        dummy_mem_array = (mem_t*)memcpy(dummy_mem_array,v->Lvalue,sizeof(mem_t));

        dummy_mem_array->size = v->datatype->def_datatype->memsize;
        dummy_mem_array->offset_expr = expr_relop_equ_addop_mult(v->Lvalue->offset_expr,OP_PLUS,expr_from_hardcoded_int(i));

        dummy_var_array = (var_t*)malloc(sizeof(var_t));
        dummy_var_array->id_is = ID_VAR;
        dummy_var_array->datatype = v->datatype->def_datatype;
        dummy_var_array->name = "__internal_dummy_variable_for_array_assignment__";
        dummy_var_array->scope = v->scope;
        dummy_var_array->cond_assign = NULL;
        dummy_var_array->Lvalue = dummy_mem_array;

        dummy_mem_l = (mem_t*)malloc(sizeof(mem_t));
        dummy_mem_l = (mem_t*)memcpy(dummy_mem_l,l->var->Lvalue,sizeof(mem_t));

        dummy_mem_l->size = l->var->datatype->def_datatype->memsize;
        dummy_mem_l->offset_expr = expr_relop_equ_addop_mult(l->var->Lvalue->offset_expr,OP_PLUS,expr_from_hardcoded_int(i));

        dummy_var_l = (var_t*)malloc(sizeof(var_t));
        dummy_var_l->id_is = ID_VAR;
        dummy_var_l->datatype = l->var->datatype->def_datatype;
        dummy_var_l->name = "__internal_dummy_variable_for_array_assignment__";
        dummy_var_l->scope = l->var->scope;
        dummy_var_l->cond_assign = NULL;
        dummy_var_l->Lvalue = dummy_mem_l;

        dummy_expr = expr_from_variable(dummy_var_l);

        new_stmt = link_ir_to_ir(new_ir_assign(dummy_var_array,dummy_expr),new_stmt);
    }
    return new_stmt;
}

ir_node_t *expand_record_assign(var_t *v,expr_t *l) {
    var_t *dummy_var_record;
    mem_t *dummy_mem_record;

    var_t *dummy_var_l;
    mem_t *dummy_mem_l;

    expr_t *dummy_expr;

    int i;

    ir_node_t *new_stmt;

    new_stmt = NULL;
    for(i=0; i<v->datatype->field_num; i++) {
        dummy_mem_record = (mem_t*)malloc(sizeof(mem_t));
        dummy_mem_record = (mem_t*)memcpy(dummy_mem_record,v->Lvalue,sizeof(mem_t));

        dummy_mem_record->size = v->datatype->field_datatype[i]->memsize;
        dummy_mem_record->offset_expr = expr_relop_equ_addop_mult(v->Lvalue->offset_expr,OP_PLUS,expr_from_hardcoded_int(v->datatype->field_offset[i]));

        dummy_var_record = (var_t*)malloc(sizeof(var_t));
        dummy_var_record->id_is = ID_VAR;
        dummy_var_record->datatype = v->datatype->field_datatype[i];
        dummy_var_record->name = "__internal_dummy_variable_for_record_assignment__";
        dummy_var_record->scope = v->scope;
        dummy_var_record->cond_assign = NULL;
        dummy_var_record->Lvalue = dummy_mem_record;

        dummy_mem_l = (mem_t*)malloc(sizeof(mem_t));
        dummy_mem_l = (mem_t*)memcpy(dummy_mem_l,l->var->Lvalue,sizeof(mem_t));

        dummy_mem_l->size = l->var->datatype->field_datatype[i]->memsize;
        dummy_mem_l->offset_expr = expr_relop_equ_addop_mult(l->var->Lvalue->offset_expr,OP_PLUS,expr_from_hardcoded_int(l->var->datatype->field_offset[i]));

        dummy_var_l = (var_t*)malloc(sizeof(var_t));
        dummy_var_l->id_is = ID_VAR;
        dummy_var_l->datatype = l->var->datatype->field_datatype[i];
        dummy_var_l->name = "__internal_dummy_variable_for_record_assignment__";
        dummy_var_l->scope = l->var->scope;
        dummy_var_l->cond_assign = NULL;
        dummy_var_l->Lvalue = dummy_mem_l;

        dummy_expr = expr_from_variable(dummy_var_l);

        new_stmt = link_ir_to_ir(new_ir_assign(dummy_var_record,dummy_expr),new_stmt);
    }
    return new_stmt;
}

ir_node_t *backpatch_ir_cond(ir_node_t *ir_cond,ir_node_t *ir_true,ir_node_t *ir_false) {
    //if (!ir_true->label) {
    //    ir_true->label = new_label_unique("_CHECK_");
    //}

    //if (!ir_false->label) {
    //    ir_false->label = new_label_unique("_CHECK_");
    //}

    switch (ir_cond->op_rval) {
    case RELOP_B:
    case RELOP_BE:
    case RELOP_L:
    case RELOP_LE:
    case RELOP_NE:
    case RELOP_EQU:
        //set the branch ir_node
        if (!ir_true->label) {
            ir_true->label = new_label_unique("CHECK");
        }

        ir_cond->last->ir_goto = ir_true;
        return ir_cond;
    case OP_AND:
        //if (!ir_cond->ir_rval->label) {
        //    ir_cond->ir_rval->label = new_label_unique("IF_AND_1");
        //}

        //if (!ir_cond->ir_rval2->label) {
        //    ir_cond->ir_rval2->label = new_label_unique("IF_AND_2");
        //}

        //invert the first comparison and jump to false
        //pseudo assembly of logical AND
        //
        //convert this:
        //
        //               beq a,b, goto: next_check
        //               jump false
        // next_check:   beq c,d, goto true
        // false:        __false__
        //               jump exit_branch
        // true:         __true__
        // exit_branch:
        //
        //to this:
        //
        //               bne a,b, goto false
        //               beq c,d, goto true
        // false:         __false__
        //               jump exit_branch
        // true:         __true__
        // exit_branch:

        //ir_cond->ir_rval->last->ir_goto = ir_false;
        //ir_cond->ir_rval2->last->ir_goto = ir_true;

        ir_cond->ir_rval2 = backpatch_ir_cond(ir_cond->ir_rval2,ir_true,ir_false);

        switch (ir_cond->ir_rval->op_rval) {
        case RELOP_B:
        case RELOP_BE:
        case RELOP_L:
        case RELOP_LE:
        case RELOP_NE:
        case RELOP_EQU:
            //printf("%s --> %s\n",op_literal(ir_cond->ir_rval->op_rval),op_literal(op_invert_cond(ir_cond->ir_rval->op_rval)));
            ir_cond->ir_rval->op_rval =  op_invert_cond(ir_cond->ir_rval->op_rval);
            ir_cond->ir_rval = backpatch_ir_cond(ir_cond->ir_rval,ir_false,ir_cond->ir_rval2);
            break;
        case OP_AND:
            ir_cond->ir_rval = backpatch_ir_cond(ir_cond->ir_rval,ir_false,ir_cond->ir_rval2);
            break;
        case OP_OR:
            ir_cond->ir_rval = backpatch_ir_cond(ir_cond->ir_rval,ir_cond->ir_rval2,ir_false);
            break;
        default:
            break;
        }

        break;
    case OP_OR:
        //if (!ir_cond->ir_rval->label) {
        //    ir_cond->ir_rval->label = new_label_unique("IF_OR_1");
        //}

        //if (ir_cond->ir_rval2->label) {
        //    ir_cond->ir_rval2->label = new_label_unique("IF_OR_2");
        //}

        //ir_cond->ir_rval->last->ir_goto = ir_true;
        //ir_cond->ir_rval2->last->ir_goto = ir_true;

        ir_cond->ir_rval = backpatch_ir_cond(ir_cond->ir_rval,ir_true,ir_false);
        ir_cond->ir_rval2 = backpatch_ir_cond(ir_cond->ir_rval2,ir_true,ir_false);
        break;
    case OP_NOT:
    default:
        die("UNEXPECTED ERROR: ir_rval_to_ir_cond: bad operator");
    }
    return ir_cond;
}

var_t *variable_from_comp_datatype_element(var_t *var) {
    expr_t *relative_offset;
    expr_t *final_offset;
    expr_t *cond;
    expr_t *final_cond;

    mem_t *new_mem;
    mem_t *base_Lvalue;

    info_comp_t *comp;
    var_t *base;
    expr_list_t *index;

    if (var->Lvalue) {
        return var;
    }

    //we must create the final Lvalue
    final_offset = expr_from_hardcoded_int(0);
    final_cond = expr_from_hardcoded_int(0);

    comp = var->from_comp;
    while (comp) {
        base = comp->base;
        index = comp->index_list;
        base_Lvalue = comp->base->Lvalue;

        switch (base->datatype->is) {
        case TYPE_ARRAY:
            if (valid_expr_list_for_array_reference(base->datatype,index)) {
                relative_offset = make_array_reference(index,base->datatype);
                cond = make_array_bound_check(index,base->datatype);
                final_cond = expr_relop_equ_addop_mult(final_cond,OP_PLUS,cond);
            } else {
                //static bound checks failed in IR level
                //the array reference was valid in the frontend, see datatypes.c: reference_to_array_element()
                //the loop optimizer has bugs
                //bad optimizer
                die("INTERNAL_ERROR: we generate out of bounds array references");
            }
            break;
        case TYPE_RECORD:
            relative_offset = expr_from_hardcoded_int(base->datatype->field_offset[comp->element]);
            break;
        default:
            die("UNEXPECTED_ERROR: no array/record in variable_from_comp_datatype_element()");
        }

        final_offset = expr_relop_equ_addop_mult(final_offset,OP_PLUS,relative_offset);

        comp = base->from_comp;
    }

    var->cond_assign = final_cond;

    new_mem = (mem_t*)malloc(sizeof(mem_t));
    new_mem = (mem_t*)memcpy(new_mem,base_Lvalue,sizeof(mem_t));
    new_mem->offset_expr = final_offset;
    new_mem->size = var->datatype->memsize;

    return var;
}
