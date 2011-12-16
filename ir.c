#include <stdlib.h>

#include "build_flags.h"
#include "semantics.h"
#include "scope.h"
#include "symbol_table.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "mem_reg.h"
#include "ir.h"
#include "ir_toolbox.h"

#include "err_buff.h"

ir_node_t *ir_root_module[MAX_NUM_OF_MODULES];
int ir_root_module_empty;
int ir_root_module_current;
ir_node_t *ir_root;

data_t datatype_max_sizeof_set; //array of chars with 16 elements (max size of sets in bytes)
var_t *ll;	//left result
var_t *rr;	//right result
var_t *x;	//protected result

ir_node_t *expand_array_assign(var_t *v,expr_t *l);
ir_node_t *expand_record_assign(var_t *v,expr_t *l);

void init_ir() {
    int i;

    for(i=0; i<MAX_NUM_OF_MODULES; i++) {
        ir_root_module[i] = NULL;
    }

    ir_root_module[0] = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_root_module[0]->label = new_label_literal("main");
    ir_root_module_empty = MAX_NUM_OF_MODULES;
    ir_root_module_current = 0;
    ir_root = ir_root_module[0];

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

void new_module(func_t *subprogram) {
    subprogram->label = new_label_subprogram(subprogram->func_name);
    ir_root_module_current++;
    ir_root_module[ir_root_module_current] = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_root_module[ir_root_module_current]->label = subprogram->label;
    ir_root_module_empty--;
    ir_root = ir_root_module[ir_root_module_current];
}

void return_to_previous_module() {
    if (ir_root_module_current==0) {
        ir_root = ir_root_module[0];
    }
    else {
        ir_root = ir_root_module[--ir_root_module_current];
    }
}

void check_for_return_value(func_t *subprogram,ir_node_t *body) {
    if (body->last_stmt->return_point==0) {
        sprintf(str_err,"ERROR: control reaches end of function '%s' without return value",subprogram->func_name);
        yyerror(str_err);
    }
}

ir_node_t *new_ir_node_t(ir_node_type_t node_type) {
    ir_node_t *new_node;

    new_node = (ir_node_t*)malloc(sizeof(ir_node_t));
    new_node->node_type = node_type;

    new_node->ir_lval = NULL;
    new_node->ir_rval = NULL;
    new_node->ir_rval2 = NULL;
    new_node->ir_cond = NULL;
    new_node->prev_stmt = new_node;
    new_node->last_stmt = new_node;
    new_node->next_stmt = NULL;
    new_node->label = NULL;
    new_node->jump_label = NULL;
    new_node->lval = NULL;
    new_node->return_point = 0;
    return new_node;
}

void link_stmt_to_tree(ir_node_t *new_node) {
    link_stmt_to_stmt(new_node,ir_root);
}

ir_node_t *link_stmt_to_stmt(ir_node_t *child,ir_node_t *parent) {
    //return the head of the linked list
    if (parent && child) {
        //if child's return_point is not set, propagate the parent's return_point value
        if (child->last_stmt->return_point==0) {
            child->last_stmt->return_point = parent->last_stmt->return_point;
        }
        parent->last_stmt->next_stmt = child;
        child->prev_stmt = parent->last_stmt;
        parent->last_stmt = child->last_stmt;
        return parent;
    } else if (!parent) {
        return child;
    } else {
        return parent;
    }
}

ir_node_t *new_assign_stmt(var_t *v, expr_t *l) {
    ir_node_t *new_stmt;

    func_t *scope_owner;

    //check for valid assignment
    if (!v || v->id_is==ID_LOST) {
        //parse errors, error is printed from the 'variable' rule
        return NULL;
    } else if (!l) {
#if BISON_DEBUG_LEVEL >= 1
        //should never reach here
        yyerror("ERROR: null expression in assignment (debugging info)");
#endif
        return NULL;
    }

    if (l->expr_is==EXPR_LOST) {
        return NULL;
    }

    if (v->id_is == ID_VAR_GUARDED) {
        sprintf(str_err,"ERROR: forbidden assignment to '%s' which controls a `for` statement",v->name);
        yyerror(str_err);
        return NULL;
    }

    if (v->id_is != ID_RETURN && v->id_is != ID_VAR) {
        sprintf(str_err,"ERROR: trying to assign to symbol '%s' which is not a variable",v->name);
        yyerror(str_err);
        return NULL;
    }

    if (v->id_is==ID_RETURN) {
        scope_owner = get_current_scope_owner();
        if (scope_owner->return_value->scope!=get_current_scope()) {
            //v->name is the same with function's name because that's how functions return their value
            sprintf(str_err,"ERROR: function '%s' asigns return value of function '%s'",v->name,scope_owner->func_name);
            yyerror(str_err);
            return NULL;
        }
    }

    if (!check_assign_similar_comp_datatypes(v->datatype,l->datatype)) {
        sprintf(str_err,"ERROR: assignment to '%s' of type '%s' with type '%s'",v->name,v->datatype->data_name,l->datatype->data_name);
        yyerror(str_err);
        return NULL;
    }

    if (TYPE_IS_STRING(v->datatype)) {
        //strings terminate with 0, copy until array is full
        new_stmt = new_ir_node_t(NODE_ASSIGN_STRING);
        new_stmt->ir_lval = calculate_lvalue(v);
        new_stmt->ir_lval2 = calculate_lvalue(l->var);
        return new_stmt;
    }

    switch (v->datatype->is) {
    case TYPE_INT:
        if (l->datatype->is!=TYPE_INT) {
	    l->convert_to = SEM_INTEGER;
        }
        break;
    case TYPE_REAL:
        if (l->datatype->is!=TYPE_REAL) {
	    l->convert_to = SEM_REAL;
        }
        break;
    case TYPE_BOOLEAN:
        if (l->datatype->is!=TYPE_BOOLEAN) {
	    yyerror("ERROR: assignment to boolean with nonboolean datatype");
	    return NULL;
        }
        break;
    case TYPE_CHAR:
        if (l->datatype->is!=TYPE_CHAR) {
#warning INFO: allow the next only if we can assign scalar types to chars
            //v->cond_assign = make_ASCII_bound_checks(v,l);
        }
        break;
    case TYPE_ENUM:
        //enumerations are integers
        //assign for enumerations
#warning INFO: allow the next only if we can assign scalar types to enumerations
        //v->cond_assign = make_enum_subset_bound_checks(v,l);
        break;
    case TYPE_SUBSET:
        v->cond_assign = make_enum_subset_bound_checks(v,l);
        break;
    case TYPE_SET:
        new_stmt = new_ir_node_t(NODE_ASSIGN_SET);
        new_stmt->ir_lval = calculate_lvalue(v);
        new_stmt->ir_lval2 = create_bitmap(l);
        return new_stmt;
    case TYPE_ARRAY:
        if (v->datatype==l->datatype) {
	    //explicit datatype match, optimize: use memcopy
	    //reminder: we do not allow signed arrays in expressions, see expr_sign() in expressions.c
	    new_stmt = new_ir_node_t(NODE_ASSIGN_MEMCPY);
	    new_stmt->ir_lval = calculate_lvalue(v);
	    new_stmt->ir_lval2 = calculate_lvalue(l->var);
        } else {
	    new_stmt = expand_array_assign(v,l);
        }
        return new_stmt;
    case TYPE_RECORD:
        if (v->datatype==l->datatype) {
	    //explicit datatype match, optimize: use memcopy
	    //reminder: we do not allow signed arrays in expressions, see expr_sign() in expressions.c
	    new_stmt = new_ir_node_t(NODE_ASSIGN_MEMCPY);
	    new_stmt->ir_lval = calculate_lvalue(v);
	    new_stmt->ir_lval2 = calculate_lvalue(l->var);
        } else {
	    new_stmt = expand_record_assign(v,l);
        }
        return new_stmt;
    case TYPE_VOID: //keep the compiler happy
        return NULL;
    }

    /**** some common assign actions */

    new_stmt = new_ir_node_t(NODE_ASM_SAVE);
    new_stmt->ir_lval = calculate_lvalue(v);
    new_stmt->ir_rval = expr_tree_to_ir_tree(l);

    if (v->id_is==ID_RETURN) {
        new_stmt->return_point = 1;
    }

    if (v->cond_assign) {
        //we must do some checks before the assignment, convert to branch node
        new_stmt = new_if_stmt(v->cond_assign,new_stmt,NULL);
    }

    return new_stmt;
}

ir_node_t *new_if_stmt(expr_t *cond,ir_node_t *true_stmt,ir_node_t *false_stmt) {
    ir_node_t *if_node;
    ir_node_t *jump_exit_branch;
    ir_node_t *ir_exit_if;

    if (!cond) {
        yyerror("UNEXPECTED_ERROR: 63-1");
        exit(EXIT_FAILURE);
    }

    if (cond->datatype->is!=TYPE_BOOLEAN || !true_stmt) {
        //parse errors or empty if statement, ignore statement
        return NULL;
    }

    if (cond->expr_is==EXPR_HARDCODED_CONST) {
        //ignore if statement, we already know the result
        if (cond->ival) {
            return true_stmt;  //TRUE
        } else {
            return false_stmt; //FALSE
        }
    }

    ir_exit_if = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_exit_if->label = new_label_unique("IF_EXIT");

    if_node = new_ir_node_t(NODE_BRANCH);

    //we implement branches by checking the !cond, see below
    cond = expr_orop_andop_notop(NULL,OP_NOT,cond);
    if_node->ir_cond = expr_tree_to_ir_tree(cond);

    if (true_stmt && false_stmt) {
        /* pseudo assembly
           if !cond goto: TRUE_STMT
           FALSE_STMT
           goto: EXIT_LABEL
           TRUE_STMT
           EXIT_LABEL
        */

        true_stmt->label = new_label_unique("TRUE_STMT");
        if_node->jump_label = true_stmt->label;

	jump_exit_branch = jump_to(ir_exit_if->label);
        false_stmt = link_stmt_to_stmt(jump_exit_branch,false_stmt);

	//propagate return_point to the last statement
	if (true_stmt->last_stmt->return_point && false_stmt->last_stmt->return_point) {
            ir_exit_if->return_point = 1;
	}
    } else {
        // only true_stmt
        /* pseudo assembly
           if !cond goto: EXIT_LABEL
           TRUE_STMT
           EXIT_LABEL
        */
        if_node->jump_label = ir_exit_if->label;
    }

    true_stmt = link_stmt_to_stmt(ir_exit_if,true_stmt); //true_stmt always exists
    if_node = link_stmt_to_stmt(false_stmt,if_node);     //this ignores false_stmt if  NULL
    if_node = link_stmt_to_stmt(true_stmt,if_node);

    return if_node;
}

ir_node_t *new_while_stmt(expr_t *cond,ir_node_t *true_stmt) {
    ir_node_t *while_node;
    ir_node_t *jump_loop_branch;

    if (!cond || !true_stmt) {
        //parse errors or empty while statement, ignore statement
        return NULL;
    }

    /* pseudo assembly
       LABEL_ENTER
       new_if_stmt(cond,true_stmt);
     */

    jump_loop_branch = jump_to(new_label_unique("WHILE_ENTER"));
    true_stmt = link_stmt_to_stmt(jump_loop_branch,true_stmt);

    while_node = new_if_stmt(cond,true_stmt,NULL);
    while_node->label = jump_loop_branch->jump_label;

    return while_node;
}

ir_node_t *new_for_stmt(char *guard_var,iter_t *range,ir_node_t *true_stmt) {
    sem_t *sem_guard;
    var_t *var_from_guarded;

    ir_node_t *dark_init_for;
    ir_node_t *dark_cond_step; //append this to true_stmt
    ir_node_t *for_node;

    expr_t *expr_guard;
    expr_t *expr_step;
    expr_t *left_cond;
    expr_t *right_cond;
    expr_t *total_cond;

    sem_guard = sm_find(guard_var);

    if ((!sem_guard || sem_guard->id_is!=ID_VAR_GUARDED) || !range || !true_stmt) {
        //parse errors or empty for_statement, ignore statement
        return NULL;
    }

    /* pseudo assembly
       INIT_FOR
       new_while_stmt(total_cond,true_stmt);
     */

    var_from_guarded = new_normal_variable_from_guarded(sem_guard->var);
    expr_guard = expr_from_variable(var_from_guarded);

    left_cond = expr_relop_equ_addop_mult(range->start,RELOP_LE,expr_guard);
    right_cond = expr_relop_equ_addop_mult(expr_guard,RELOP_LE,range->stop);
    total_cond = expr_orop_andop_notop(left_cond,OP_AND,right_cond);

    dark_init_for = new_assign_stmt(var_from_guarded,range->start);
    dark_init_for->label = new_label_unique("FOR_ENTER");

    expr_step = expr_relop_equ_addop_mult(expr_guard,OP_PLUS,range->step);
    dark_cond_step = new_assign_stmt(expr_guard->var,expr_step);

    true_stmt = link_stmt_to_stmt(dark_cond_step,true_stmt);

    for_node = new_while_stmt(total_cond,true_stmt);
    for_node = link_stmt_to_stmt(for_node,dark_init_for);

    return for_node;
}

ir_node_t *new_with_stmt(ir_node_t *body) {
    //if the variable is a record return the body of the statement
    //the body is a comp_statement
    return body;
}

ir_node_t *new_procedure_call(char *id,expr_list_t *list) {
    sem_t *sem_1;
    ir_node_t *dark_init_node;
    ir_node_t *new_proc_call;

    sem_1 = sm_find(id);
    if (sem_1) {
        //it is possible to call a subprogram before defining its body, so check for _FORWARDED_ subprograms too
        //if the sub_type is valid, continue as the subprogram args are correct, to avoid false error messages afterwards
        if (sem_1->id_is == ID_FUNC || sem_1->id_is == ID_FORWARDED_FUNC) {
            sprintf(str_err,"ERROR: invalid '%s' function call, expected procedure",id);
            yyerror(str_err);
        } else if (sem_1->id_is == ID_PROC || sem_1->id_is == ID_FORWARDED_PROC) {
            dark_init_node = prepare_stack_for_call(sem_1->subprogram,list);
            if (dark_init_node) {
                new_proc_call = jump_and_link_to(sem_1->subprogram->label);
                new_proc_call = link_stmt_to_stmt(new_proc_call,dark_init_node);
                return new_proc_call;
            }
            //we had parse errors
        } else {
            yyerror("ID is not a subprogram.");
        }
    } else {
        if (!sm_find_lost_symbol(id)) {
            sm_insert_lost_symbol(id);
            sprintf(str_err,"ERROR: undeclared subprogram '%s'",id);
            yyerror(str_err);
        }
    }
    return NULL;
}

ir_node_t *new_comp_stmt(ir_node_t *body) {
    return body;
}

/** Assume that read and write statements have meaning only for standard types and STRINGS
 * these are the only serializeable datatypes
 * booleans are considered chars here and we must check their value after we read them
 * chars are considered STRINGS of size 1
 */
ir_node_t *new_read_stmt(var_list_t *list) {
    int i;
    int error=0;
    ir_node_t *new_ir;
    ir_node_t *read_stmt;
    ir_node_t *ir_result_lval;
    var_t *v;
    data_t *d;

    //print every possible error (if any) before returning NULL
    for(i=0;i<MAX_VAR_LIST-list->var_list_empty;i++) {
        v = list->var_list[i];
        if (v->id_is==ID_LOST) {
            error++;
        } else if (v->id_is==ID_VAR_GUARDED) {
            sprintf(str_err,"ERROR: in read, control variable '%s' of 'for statement' is read only",v->name);
            yyerror(str_err);
            error++;
        } else if (v->id_is==ID_CONST) {
            sprintf(str_err,"ERROR: in read, trying to change constant '%s'",v->name);
            yyerror(str_err);
            error++;
        } else if (v->id_is==ID_RETURN) {
            sprintf(str_err,"ERROR: in read, return value '%s' can be set only with assignment",v->name);
            yyerror(str_err);
            error++;
        } else if (!TYPE_IS_STANDARD(v->datatype) && !TYPE_IS_STRING(v->datatype)) {
            sprintf(str_err,"ERROR: in read, expected standard type '%s' or string, istead of '%s'",v->name,v->datatype->data_name);
            yyerror(str_err);
            error++;
        }
    }

    if (error) {
        return NULL;
    }

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
            new_ir = link_stmt_to_stmt(new_ir,read_stmt);
            break;
        default:
            yyerror("UNEXPECTED_ERROR: 44-44");
            exit(EXIT_FAILURE);
        }
    }
    return read_stmt;
}

ir_node_t *new_write_stmt(expr_list_t *list) {
    int i;
    int error=0;
    ir_node_t *new_ir;
    ir_node_t *write_stmt;
    expr_t *l;
    data_t *d;

    //print every possible error
    for(i=0;i<MAX_EXPR_LIST-list->expr_list_empty;i++) {
        l = list->expr_list[i];
        if (l->expr_is==EXPR_LOST) {
            error++;
        }
        else if (l->expr_is==EXPR_SET || EXPR_NULL_SET) {
            sprintf(str_err,"ERROR: in write, can only print standard types or strings, this is a set");
            yyerror(str_err);
            error++;
        }
        else if (!TYPE_IS_STANDARD(l->datatype) && l->expr_is!=EXPR_STRING) {
            //EXPR_RVAL, EXPR_HARDCODED_CONST, EXPR_LVAL, EXPR_STRING
            sprintf(str_err,"ERROR: in write, '%s' is not standard type ('%s') or string",l->var->name,l->datatype->data_name);
            yyerror(str_err);
            error++;
        }
    }

    if (error) {
        return NULL;
    }

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
        new_ir = link_stmt_to_stmt(new_ir,write_stmt);
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
        dummy_mem_array->content_type = v->Lvalue->content_type;
        dummy_mem_array->direct_register_number = v->Lvalue->direct_register_number;
        dummy_mem_array->seg_offset = v->Lvalue->seg_offset;
        dummy_mem_array->segment = v->Lvalue->segment;
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
        dummy_mem_l->content_type = l->var->Lvalue->content_type;
        dummy_mem_l->direct_register_number = l->var->Lvalue->direct_register_number;
        dummy_mem_l->seg_offset = l->var->Lvalue->seg_offset;
        dummy_mem_l->segment = l->var->Lvalue->segment;
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

        new_stmt = link_stmt_to_stmt(new_assign_stmt(dummy_var_array,dummy_expr),new_stmt);
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
        dummy_mem_record->content_type = v->Lvalue->content_type;
        dummy_mem_record->direct_register_number = v->Lvalue->direct_register_number;
        dummy_mem_record->seg_offset = v->Lvalue->seg_offset;
        dummy_mem_record->segment = v->Lvalue->segment;
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
        dummy_mem_l->content_type = l->var->Lvalue->content_type;
        dummy_mem_l->direct_register_number = l->var->Lvalue->direct_register_number;
        dummy_mem_l->seg_offset = l->var->Lvalue->seg_offset;
        dummy_mem_l->segment = l->var->Lvalue->segment;
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

        new_stmt = link_stmt_to_stmt(new_assign_stmt(dummy_var_record,dummy_expr),new_stmt);
    }
    return new_stmt;
}
