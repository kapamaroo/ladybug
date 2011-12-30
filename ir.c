#include <stdlib.h>
#include <string.h> //memcpy()

#include "build_flags.h"
#include "semantics.h"
#include "scope.h"
#include "symbol_table.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "mem.h"
#include "ir.h"
#include "ir_toolbox.h"
#include "bitmap.h"
#include "err_buff.h"

unsigned long unique_virt_reg;

ir_node_t *ir_root_tree[MAX_NUM_OF_MODULES];
int ir_root_tree_current;

ir_node_t *new_ir_assign_str(var_t *v, expr_t *l);
ir_node_t *new_ir_assign_expr(var_t *v, expr_t *l);
ir_node_t *expand_array_assign(var_t *v,expr_t *l);
ir_node_t *expand_record_assign(var_t *v,expr_t *l);

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

void init_ir() {
    int i;

    //first of all initialize the register counter
    unique_virt_reg = 40; //to recognize the virtual registers easier

    for(i=0; i<MAX_NUM_OF_MODULES; i++) {
        ir_root_tree[i] = NULL;
    }

    ir_root_tree[0] = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_root_tree[0]->label = new_label_literal("main");
    ir_root_tree_current = 0;

    init_bitmap();
}

void new_ir_tree(char *label) {
    ir_node_t *ir_new;

    ir_new = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_new->label = label;

    ir_root_tree_current++;
    ir_root_tree[ir_root_tree_current] = ir_new;
}

void return_to_previous_ir_tree() {
    if (ir_root_tree_current==0) {
        return;
    }

    ir_root_tree_current--;
}

void link_ir_to_tree(ir_node_t *new_node) {
    ir_root_tree[ir_root_tree_current] =
        link_ir_to_ir(new_node,ir_root_tree[ir_root_tree_current]);
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

    /*
    switch(node_type) {
    case NODE_DUMMY_LABEL:
    case NODE_LOST_NODE:
    case NODE_BRANCH:
    case NODE_JUMP_LINK:
    case NODE_JUMP:
    case NODE_RETURN_SUBPROGRAM:
    case NODE_INPUT_INT:
    case NODE_INPUT_REAL:
    case NODE_INPUT_BOOLEAN:
    case NODE_INPUT_CHAR:
    case NODE_INPUT_STRING:
    case NODE_OUTPUT_INT:
    case NODE_OUTPUT_REAL:
    case NODE_OUTPUT_BOOLEAN:
    case NODE_OUTPUT_CHAR:
    case NODE_OUTPUT_STRING:
    case NODE_MEMCPY:
    case NODE_ASSIGN:
    case NODE_ASSIGN_SET:
    case NODE_ASSIGN_STRING:
    case NODE_INIT_NULL_SET: //assign zero
    case NODE_HARDCODED_LVAL:
    case NODE_HARDCODED_RVAL:
        //do not give real reg here
        break;
    case NODE_LOAD:
    case NODE_CONVERT_TO_INT:
    case NODE_CONVERT_TO_REAL:
    case NODE_BINARY_AND:
    case NODE_BINARY_OR:
    case NODE_BINARY_NOT:
    case NODE_SHIFT_LEFT:
    case NODE_SHIFT_RIGHT:
    case NODE_RVAL:
    case NODE_LVAL:
        //new_node->reg = get_available_reg(REG_CONTENT);
        break;
    case NODE_ADD_ELEM_TO_SET:
    case NODE_ADD_ELEM_RANGE_TO_SET:
    case NODE_CHECK_INOP_BITMAPPED:
#warning do these nodes need a real register?
        break;
    }
    */

    new_node->virt_reg = ++unique_virt_reg;

    new_node->ir_lval = NULL;
    new_node->ir_lval2 = NULL;
    new_node->ir_rval = NULL;
    new_node->ir_rval2 = NULL;

    new_node->ir_cond = NULL;
    new_node->address = NULL;
    new_node->offset = NULL;
    new_node->ir_lval_dest = NULL;

    new_node->next = NULL;
    new_node->prev = NULL; //new_node;
    new_node->last = new_node;
    new_node->label = NULL;
    new_node->jump_label = NULL;
    new_node->error = NULL;
    new_node->lval = NULL;
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

    //strings terminate with 0, copy until array is full
    new_stmt = new_ir_node_t(NODE_ASSIGN_STRING);
    new_stmt->address = calculate_lvalue(v);
    new_stmt->ir_lval = calculate_lvalue(l->var);
    return new_stmt;
}

ir_node_t *new_ir_assign_expr(var_t *v, expr_t *l) {
    ir_node_t *new_stmt;

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
#warning implement me
        //we must assign either 1 or 0
        printf("IMPLEMENT ME: assign to boolean");
        exit(EXIT_FAILURE);
        break;
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
	    new_stmt = expand_record_assign(v,l);
        }
        return new_stmt;
    case TYPE_SET:
        //new_stmt = new_ir_node_t(NODE_ASSIGN);
        new_stmt = new_ir_node_t(NODE_ASSIGN_SET);
        new_stmt->address = calculate_lvalue(v);
        new_stmt->ir_lval = create_bitmap(l);
        return new_stmt;
    case TYPE_VOID: //keep the compiler happy
        printf("UNEXPECTED ERROR: TYPE_VOID in assignment\n");
        exit(EXIT_FAILURE);
    }

    /**** some common assign actions */

    new_stmt = new_ir_node_t(NODE_ASSIGN);
    new_stmt->address = calculate_lvalue(v);
    new_stmt->ir_rval = expr_tree_to_ir_tree(l);

    if (v->cond_assign) {
        //we must do some checks before the assignment, convert to branch node
        new_stmt = new_ir_if(v->cond_assign,new_stmt,NULL);
    }

    return new_stmt;
}

ir_node_t *new_ir_if(expr_t *cond,ir_node_t *true_stmt,ir_node_t *false_stmt) {
    ir_node_t *if_node;
    ir_node_t *jump_exit_branch;
    ir_node_t *ir_exit_if;

    ir_exit_if = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_exit_if->label = new_label_unique("IF_EXIT");

    if_node = new_ir_node_t(NODE_BRANCH);

    if (true_stmt && false_stmt) {
        /* pseudo assembly
           if cond goto: TRUE_STMT
           FALSE_STMT
           goto: EXIT_LABEL
           TRUE_STMT
           EXIT_LABEL
        */

        true_stmt->label = new_label_unique("TRUE_STMT");
        if_node->jump_label = true_stmt->label;

	jump_exit_branch = jump_to(ir_exit_if->label);
        false_stmt = link_ir_to_ir(jump_exit_branch,false_stmt);

    } else {
        // only true_stmt
        /* pseudo assembly
           if !cond goto: EXIT_LABEL
           TRUE_STMT
           EXIT_LABEL
        */

        cond = expr_orop_andop_notop(NULL,OP_NOT,cond);
        if_node->jump_label = ir_exit_if->label;
    }

    true_stmt = link_ir_to_ir(ir_exit_if,true_stmt); //true_stmt always exists

    if_node->ir_cond = expr_tree_to_ir_tree(cond);

    if_node = link_ir_to_ir(false_stmt,if_node);     //this ignores false_stmt if  NULL
    if_node = link_ir_to_ir(true_stmt,if_node);

    return if_node;
}

ir_node_t *new_ir_while(expr_t *cond,ir_node_t *true_stmt) {
    ir_node_t *while_node;
    ir_node_t *jump_loop_branch;

    /* pseudo assembly
       LABEL_ENTER
       new_ir_if(cond,true_stmt);
     */

    jump_loop_branch = jump_to(new_label_unique("WHILE_ENTER"));
    true_stmt = link_ir_to_ir(jump_loop_branch,true_stmt);

    while_node = new_ir_if(cond,true_stmt,NULL);
    while_node->label = jump_loop_branch->jump_label;

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

    //this is a hack, if we ever do multithreading this is not gona work
    var->id_is = ID_VAR;
    dark_init_for = new_ir_assign(var,range->start);
    dark_cond_step = new_ir_assign(var,expr_step);
    var->id_is = ID_VAR_GUARDED;

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
    ir_node_t *ir_result_lval;
    var_t *v;
    data_t *d;

    //we are error free here!
    read_stmt = NULL;
    for(i=0;i<MAX_VAR_LIST-list->var_list_empty;i++) {
        new_ir = NULL;
        v = list->var_list[i];
        d = v->datatype;
        switch (v->id_is) {
        case ID_VAR:
            ir_result_lval = calculate_lvalue(v);
            if (d->is==TYPE_INT) {
                new_ir = new_ir_node_t(NODE_INPUT_INT);
                new_ir->ir_lval = ir_result_lval;
            }
            else if (d->is==TYPE_REAL) {
                new_ir = new_ir_node_t(NODE_INPUT_REAL);
                new_ir->ir_lval = ir_result_lval;
            }
            else if (d->is==TYPE_BOOLEAN) {
                //read int and check the value
                new_ir = new_ir_node_t(NODE_INPUT_BOOLEAN);
                new_ir->ir_lval = ir_result_lval;
            }
            else if (d->is==TYPE_CHAR) {
                new_ir = new_ir_node_t(NODE_INPUT_CHAR);
                new_ir->ir_lval = ir_result_lval;
            }
            else if (TYPE_IS_STRING(d)) {
                new_ir = new_ir_node_t(NODE_INPUT_STRING);
                new_ir->ir_lval = ir_result_lval;
                new_ir->ival = v->datatype->dim[0]->range;
            } else {
                yyerror("UNEXPECTED_ERROR: 44-42");
                exit(EXIT_FAILURE);
            }
            new_ir = link_ir_to_ir(new_ir,read_stmt);
            break;
        default:
            yyerror("UNEXPECTED_ERROR: 44-44");
            exit(EXIT_FAILURE);
        }
    }
    return read_stmt;
}

ir_node_t *new_ir_write(expr_list_t *list) {
    int i;
    ir_node_t *new_ir;
    ir_node_t *write_stmt;
    expr_t *l;
    data_t *d;

    //we are error free here!
    write_stmt = NULL;
    for(i=0;i<MAX_EXPR_LIST-list->expr_list_empty;i++) {
        new_ir = NULL;
        l = list->expr_list[i];
        if (l->expr_is==EXPR_STRING) {
            new_ir = new_ir_node_t(NODE_OUTPUT_STRING);
            new_ir->ir_lval = calculate_lvalue(l->var);
        }
        else if (l->expr_is==EXPR_RVAL || l->expr_is==EXPR_HARDCODED_CONST || l->expr_is==EXPR_LVAL) {
            d = l->datatype;

            if (d->is==TYPE_INT) {
                new_ir = new_ir_node_t(NODE_OUTPUT_INT);
                new_ir->ir_rval = expr_tree_to_ir_tree(l);
            }
            else if (d->is==TYPE_REAL) {
                new_ir = new_ir_node_t(NODE_OUTPUT_REAL);
                new_ir->ir_rval = expr_tree_to_ir_tree(l);
            }
            else if (d->is==TYPE_BOOLEAN) {
                new_ir = new_ir_node_t(NODE_OUTPUT_BOOLEAN);
                new_ir->ir_rval = expr_tree_to_ir_tree(l);
            }
            else if (d->is==TYPE_CHAR) {
                new_ir = new_ir_node_t(NODE_OUTPUT_CHAR);
                new_ir->ir_rval = expr_tree_to_ir_tree(l);
            }
            else {
                yyerror("UNEXPECTED_ERROR: 44-46");
                exit(EXIT_FAILURE);
            }
        }
        else {
            yyerror("UNEXPECTED_ERROR: 44-45");
            exit(EXIT_FAILURE);
        }
        new_ir = link_ir_to_ir(new_ir,write_stmt);
    }
    return write_stmt;
}

ir_node_t *jump_and_link_to(char *jump_label) {
    ir_node_t *new_jump_link;

    new_jump_link = new_ir_node_t(NODE_JUMP_LINK);
    new_jump_link->jump_label = jump_label;
    return new_jump_link;
}

ir_node_t *jump_to(char *jump_label) {
    ir_node_t *new_jump_link;

    new_jump_link = new_ir_node_t(NODE_JUMP);
    new_jump_link->jump_label = jump_label;
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

ir_node_t *new_lost_ir_node(char *error) {
    ir_node_t *new_ir;

    new_ir = new_ir_node_t(NODE_LOST_NODE);
    new_ir->error = error;

    return new_ir;
}
