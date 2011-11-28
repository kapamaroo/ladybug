#include <stdlib.h>

#include "build_flags.h"
#include "semantics.h"
#include "symbol_table.h"
#include "semantic_routines.h"
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
    new_node->exit_branch_label = NULL;
    new_node->lval = NULL;

    if (node_type==NODE_RETURN_FUNC || node_type==NODE_RETURN_PROC) {
        new_node->return_point = 1;
    }
    else {
        new_node->return_point = 0;
    }

    return new_node;
}

void link_stmt_to_tree(ir_node_t *new_node) {
    if (ir_root->last_stmt->return_point) {
        yyerror("WARNING: ommiting dead code after control return");
        return;
    }

    link_stmt_to_stmt(new_node,ir_root);
}

ir_node_t *link_stmt_to_stmt(ir_node_t *child,ir_node_t *parent) {
    //return the head of the linked list
    if (parent && child) {
        if (parent->last_stmt->return_point) {
            yyerror("WARNING: ommiting dead code after control return");
            return parent;
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

ir_node_t *link_stmt_to_stmt_anyway(ir_node_t *child,ir_node_t *parent) {
    //return the head of the linked list
    if (parent && child) {
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
    ir_node_t *new_ir_lval;
    ir_node_t *new_ir_rval;

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

    if (v->id_is != ID_RETURN && v->id_is != ID_VAR && v->id_is!=ID_VAR_PTR) {
        sprintf(str_err,"ERROR: trying to assign to symbol '%s' which is not a variable",v->name);
        yyerror(str_err);
        return NULL;
    }

    if (v->id_is==ID_RETURN && v->scope!=get_current_scope()) {
        scope_owner = get_current_scope_owner();
        sprintf(str_err,"ERROR: assignment to '%s' which is return value of scope '%s'",v->name,scope_owner->func_name);
        yyerror(str_err);
        return NULL;
    }

    if (v->id_is == ID_VAR_GUARDED) {
        sprintf(str_err,"ERROR: forbidden assignment to '%s' which controls a `for` statement",v->name);
        yyerror(str_err);
        return NULL;
    }

    if (v->id_is==ID_VAR_PTR && v->Lvalue->content_type==PASS_REF && l->expr_is!=EXPR_LVAL) {
        sprintf(str_err,"ERROR: incompatible assignment by reference to parameter '%s'",v->name);
        yyerror(str_err);
        return NULL;
    }

    if (!check_assign_similar_comp_datatypes(v->datatype,l->datatype)) {
        sprintf(str_err,"ERROR: invalid assignment to '%s' of type '%s' with type '%s'",v->name,v->datatype->data_name,l->datatype->data_name);
        yyerror(str_err);
        return NULL;
    }

    if (l->expr_is==EXPR_STRING) { //string assignment
        if (TYPE_IS_STRING(v->datatype)) {
            //strings terminate with 0, copy until array is full
            new_stmt = new_ir_node_t(NODE_ASSIGN_STRING);
            new_stmt->ir_lval = calculate_lvalue(v);
            new_stmt->ir_lval2 = calculate_lvalue(l->var);
            return new_stmt;
        } else {
            yyerror("ERROR: invalid string assignment");
            return NULL;
        }
    } else if (l->expr_is==EXPR_SET || l->expr_is==EXPR_NULL_SET ||
               (l->expr_is==EXPR_LVAL && l->datatype->is==TYPE_SET)) { //set assignment
        if (v->datatype->is==TYPE_SET &&
                ((l->expr_is==EXPR_LVAL && v->datatype==l->datatype) ||
                 ((l->expr_is==EXPR_SET || l->expr_is==EXPR_NULL_SET) && v->datatype->def_datatype==l->datatype))) {
            new_stmt = new_ir_node_t(NODE_ASSIGN_SET);
            new_stmt->ir_lval = calculate_lvalue(v);
            new_stmt->ir_lval2 = create_bitmap(l);
            return new_stmt;
        } else {
            if (v->id_is==ID_RETURN) {
                yyerror("ERROR: incompatible assignment to return value, expected expression of standard type");
            } else {
                sprintf(str_err,"ERROR: incompatible type assignment to '%s'",v->name);
                yyerror(str_err);
            }
            return NULL;
        }
    } else { //normal assignment with EXPR_RVAL or EXPR_LVAL or EXPR_HARDCODED_CONST
        switch (v->datatype->is) {
        case TYPE_INT:
        case TYPE_REAL:
        case TYPE_BOOLEAN:
        case TYPE_CHAR:
            if (v->datatype->is==TYPE_REAL) {
                l->convert_to = SEM_REAL;
            } else if (v->datatype->is==TYPE_INT) {
                //chars and booleans are represented as integers
                l->convert_to = SEM_INTEGER;
            } else if (v->datatype->is==TYPE_CHAR) {
#warning INFO: allow the next only if we can assign scalar types to chars
                //v->cond_assign = make_ASCII_bound_checks(v,l);
            }
            break;
        case TYPE_ENUM: //enumerations are integers
            //assign for enumerations
#warning INFO: allow the next only if we can assign scalar types to enumerations
            //v->cond_assign = make_enum_subset_bound_checks(v,l);
            break;
        case TYPE_SUBSET:
            v->cond_assign = make_enum_subset_bound_checks(v,l);
            break;
        case TYPE_SET:
            yyerror("ERROR: assignment to type of set with a non set expression");
            return NULL;
            break;
        case TYPE_ARRAY:
            if (v->datatype==l->datatype) {
                //expression is EXPR_LVAL here
                //reminder: we do not allow signed arrays in expressions, see expr_sign() in expressions.c
                new_stmt = new_ir_node_t(NODE_ASSIGN_MEMCPY);
                new_stmt->ir_lval = calculate_lvalue(v);
                new_stmt->ir_lval2 = calculate_lvalue(l->var);
            } else {
                //expression is EXPR_LVAL here
                var_t *dummy_var_array;
                mem_t *dummy_mem_array;

                var_t *dummy_var_l;
                mem_t *dummy_mem_l;

                expr_t *dummy_expr;

                int i;
                int elem_offset_num;

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

                    dummy_expr = (expr_t*)malloc(sizeof(expr_t));
                    dummy_expr->expr_is = EXPR_LVAL;
                    dummy_expr->datatype = dummy_var_l->datatype;
                    dummy_expr->var = dummy_var_l;

                    new_stmt = link_stmt_to_stmt(new_assign_stmt(dummy_var_array,dummy_expr),new_stmt);
                }
            }
            return new_stmt;
        case TYPE_RECORD:
            if (v->datatype==l->datatype) {
                //expression is EXPR_LVAL here
                //reminder: we do not allow signed arrays in expressions, see expr_sign() in expressions.c
                new_stmt = new_ir_node_t(NODE_ASSIGN_MEMCPY);
                new_stmt->ir_lval = calculate_lvalue(v);
                new_stmt->ir_lval2 = calculate_lvalue(l->var);
            } else {
                //expression is EXPR_LVAL here
                var_t *dummy_var_record;
                mem_t *dummy_mem_record;

                var_t *dummy_var_l;
                mem_t *dummy_mem_l;

                expr_t *dummy_expr;

                int i;
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

                    dummy_expr = (expr_t*)malloc(sizeof(expr_t));
                    dummy_expr->expr_is = EXPR_LVAL;
                    dummy_expr->datatype = dummy_var_l->datatype;
                    dummy_expr->var = dummy_var_l;

                    new_stmt = link_stmt_to_stmt(new_assign_stmt(dummy_var_record,dummy_expr),new_stmt);
                }
            }
            return new_stmt;
        case TYPE_VOID: //keep the compiler happy
            return NULL;
        }
    }

    //calculate lval for assignment
    new_ir_lval = calculate_lvalue(v);

    //calculate rval for assignment
    if (v->id_is==ID_VAR_PTR && v->Lvalue->content_type==PASS_REF) {
        new_ir_rval = calculate_lvalue(l->var);
    } else {
        new_ir_rval = expr_tree_to_ir_tree(l);
    }

    if (v->id_is==ID_RETURN) {
        new_stmt = new_ir_node_t(NODE_RETURN_FUNC);
    } else {
        new_stmt = new_ir_node_t(NODE_ASM_SAVE);
    }
    new_stmt->ir_lval = new_ir_lval;
    new_stmt->ir_rval = new_ir_rval;

    if (v->cond_assign) {
        //we must do some checks before the assignment, convert to branch node
        new_stmt = new_if_stmt(v->cond_assign,new_stmt,NULL);
    }

    return new_stmt;
}

ir_node_t *new_if_stmt(expr_t *cond,ir_node_t *true_stmt,ir_node_t *false_stmt) {
    ir_node_t *if_node;
    ir_node_t *jump_exit_branch;
    ir_node_t *body;
    ir_node_t *ir_dummy_label;

    char *label_enter_branch;
    char *label_exit_branch;
    char *label_false;

    if (!cond) {
        yyerror("UNEXPECTED_ERROR: 63-1");
        exit(EXIT_FAILURE);
    }

    if (cond->datatype->is!=TYPE_BOOLEAN && !true_stmt && !false_stmt) {
        //parse errors or empty if statement, ignore statement
        return NULL;
    }

    if (cond->expr_is==EXPR_HARDCODED_CONST) {
        //ignore if statement, we already know the result
        if (cond->ival) {
            //TRUE
            return true_stmt;
        } else {
            //FALSE
            return false_stmt;
        }
    }

    label_enter_branch = new_label_unique("ifbranch");
    label_exit_branch = new_label_exit_branch(label_enter_branch);

    //there is at least one of the true_stmt and false_stmt
    if_node = new_ir_node_t(NODE_BRANCH);
    //if_node->label = label_enter_branch; //we do not need a label here, maybe the while or the for statement need it
    if_node->exit_branch_label = label_exit_branch;

    ir_dummy_label = new_ir_node_t(NODE_DUMMY_LABEL);
    ir_dummy_label->label = label_exit_branch;

    body = NULL;
    if (true_stmt && false_stmt) {
        label_false = new_label_false(label_enter_branch);
        if_node->exit_branch_label = label_false; //if !cond goto false stmt
        false_stmt->label = label_false;
        jump_exit_branch = new_ir_node_t(NODE_JUMP);
        jump_exit_branch->jump_label = label_exit_branch;

        if (true_stmt->last_stmt->return_point && false_stmt->last_stmt->return_point) {
            ir_dummy_label->return_point = 1;
        }

        false_stmt = link_stmt_to_stmt(false_stmt,jump_exit_branch);
        body = link_stmt_to_stmt_anyway(true_stmt,false_stmt);

    } else if (true_stmt) {
        body = link_stmt_to_stmt(true_stmt,body);
        ir_dummy_label->return_point = 1;
    } else { //there is only the false_stmt, invert the condition
        cond = expr_orop_andop_notop(NULL,OP_NOT,cond);
        body = link_stmt_to_stmt(false_stmt,body);
    }

    body = link_stmt_to_stmt_anyway(ir_dummy_label,body);

    if_node->ir_rval = expr_tree_to_ir_tree(cond);
    if_node = link_stmt_to_stmt(body,if_node);

    return if_node;
}

ir_node_t *new_while_stmt(expr_t *cond,ir_node_t *true_stmt) {
    ir_node_t *while_node;
    ir_node_t *ir_cond;
    ir_node_t *ir_;
    ir_node_t *jump_loop_branch;

    char *label_enter_branch;
    char *label_loop_to_cond;
    //char *label_exit_branch;

    if (!cond || !true_stmt) {
        //parse errors or empty while statement, ignore statement
        return NULL;
    }

    label_enter_branch = new_label_unique("whilebranch");
    label_loop_to_cond = label_enter_branch;
    //label_exit_branch = new_label_exit_branch(label_enter_branch);

    jump_loop_branch = new_ir_node_t(NODE_JUMP);
    jump_loop_branch->jump_label = label_loop_to_cond;

    ir_ = link_stmt_to_stmt(jump_loop_branch,true_stmt);

    ir_cond = new_if_stmt(cond,ir_,NULL);
    ir_cond->label = label_loop_to_cond;
//#warning INFO: the next works ONLY because one of the false_stmt,true_stmt is NULL
    //ir_cond->exit_branch_label = label_exit_branch;

    while_node = NULL;
    while_node = link_stmt_to_stmt(ir_cond,while_node);

    return while_node;
}

ir_node_t *new_for_stmt(char *guard_var,iter_t *range,ir_node_t *true_stmt) {
    sem_t *sem_guard;
    var_t *var_from_guarded;

    ir_node_t *dark_init_for;
    ir_node_t *ir_cond;
    ir_node_t *ir_body;
    ir_node_t *dark_cond_step; //append this to true_stmt
    ir_node_t *jump_loop_branch;
    ir_node_t *for_node;

    char *label_enter_branch;
    char *label_loop_to_cond;
    //char *label_exit_branch;

    expr_t *expr_guard;
    expr_t *expr_normal_from_guard;
    expr_t *expr_step;
    expr_t *left_cond;
    expr_t *right_cond;
    expr_t *total_cond;

    sem_guard = sm_find(guard_var);

    if ((!sem_guard || sem_guard->id_is!=ID_VAR_GUARDED) || !range || !true_stmt) {
        //parse errors or empty for_statement, ignore statement
        return NULL;
    }

    label_enter_branch = new_label_unique("forbranch");
    label_loop_to_cond = new_label_loop_to_cond(label_enter_branch);
    //label_exit_branch = new_label_exit_branch(label_enter_branch);

    var_from_guarded = new_normal_variable_from_guarded(sem_guard->var);
    expr_normal_from_guard = expr_from_variable(var_from_guarded);

    expr_guard = expr_from_variable(sem_guard->var);
    left_cond = expr_relop_equ_addop_mult(range->start,RELOP_LE,expr_guard);
    right_cond = expr_relop_equ_addop_mult(expr_guard,RELOP_LE,range->stop);
    total_cond = expr_orop_andop_notop(left_cond,OP_AND,right_cond);

    dark_init_for = new_assign_stmt(var_from_guarded,range->start);
    dark_init_for->label = label_enter_branch;

    expr_step = expr_relop_equ_addop_mult(expr_normal_from_guard,OP_PLUS,range->step);
    dark_cond_step = new_assign_stmt(sem_guard->var,expr_step);

    jump_loop_branch = new_ir_node_t(NODE_JUMP);
    jump_loop_branch->jump_label = label_loop_to_cond;

    ir_body = link_stmt_to_stmt(dark_cond_step,true_stmt);
    ir_body = link_stmt_to_stmt(jump_loop_branch,true_stmt);

    ir_cond = new_if_stmt(total_cond,ir_body,NULL);
    ir_cond->label = label_loop_to_cond;
//#warning INFO: the next works ONLY because one of the false_stmt,true_stmt is NULL
    //ir_cond->exit_branch_label = label_exit_branch;

    for_node = NULL;
    for_node = link_stmt_to_stmt(dark_init_for,for_node);
    for_node = link_stmt_to_stmt(ir_cond,for_node);

    unprotect_guard_var(guard_var);
    return for_node;
}

ir_node_t *new_with_stmt(ir_node_t *body) {
    //if the variable is a record return the body of the statement
    //the body is a comp_statement

    close_last_opened_with_statement_scope();
    return body;
}

expr_t *new_function_call(char *id,expr_list_t *list) {
//functions can only be called inside expressions, so this ID must be a function, its type is its return type
    sem_t *sem_1;
    expr_t *new_expr;

    sem_1 = sm_find(id);
    if (sem_1) {
        //it is possible to call a subprogram before defining its body, so check for _FORWARDED_ subprograms too
        //if the sub_type is valid, continue as the subprogram args are correct, to avoid false error messages afterwards
        if (sem_1->id_is == ID_FUNC || sem_1->id_is == ID_FORWARDED_FUNC) {
            //else we had parse errors
            new_expr = expr_from_variable(sem_1->var);
            new_expr->expr_list = list;
            return new_expr;
        } else if (sem_1->id_is == ID_PROC || sem_1->id_is == ID_FORWARDED_PROC) {
            sprintf(str_err,"ERROR: invalid procedure call '%s', expected function",id);
            yyerror(str_err);
            return expr_from_lost_int(0); //return something to avoid unreal error messages
        } else {
            yyerror("ID is not a subprogram.");
            return expr_from_lost_int(0); //return something to avoid unreal error messages
        }
    } else {
        if (!sm_find_lost_symbol(id)) {
            sm_insert_lost_symbol(id);
            sprintf(str_err,"ERROR: undeclared subprogram '%s'",id);
            yyerror(str_err);
        }
        return expr_from_lost_int(0); //return something to avoid unreal error messages
    }
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
 * booleans are considered integers here and we must check their value after we read them
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

    //print every possible error
    for(i=0;i<MAX_VAR_LIST-list->var_list_empty;i++) {
        v = list->var_list[i];
        if (!TYPE_IS_STANDARD(v->datatype) && !TYPE_IS_STRING(v->datatype)) {
            if (v->id_is!=ID_LOST) {
                sprintf(str_err,"ERROR: read statement can only read standard types and strings,'%s' of type '%s'",v->name,v->datatype->data_name);
                yyerror(str_err);
            }
            error = 1;
        }
    }

    if (error) {
        //including EXPR_LOST
        return NULL;
    }

    read_stmt = NULL;
    for(i=0;i<MAX_VAR_LIST-list->var_list_empty;i++) {
        new_ir = NULL;
        v = list->var_list[i];
        switch (v->id_is) {
        case ID_VAR:
        case ID_VAR_PTR:
            d = v->datatype;
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
            else {
                //STRING
                //STRINGS only have one dimension
                new_ir = new_ir_node_t(NODE_INPUT_STRING);
                new_ir->ir_lval = ir_result_lval;
                new_ir->ival = v->datatype->dim[0]->range;
            }
            break;
       case ID_VAR_GUARDED:
            sprintf(str_err,"ERROR: invalid read statement, control variable '%s' of 'for statement' is read only",v->name);
            yyerror(str_err);
            break;
        case ID_CONST:
            sprintf(str_err,"ERROR: invalid read staetment, trying to change constant '%s'",v->name);
            yyerror(str_err);
            break;
        case ID_RETURN:
            sprintf(str_err,"ERROR: invalid read statement, trying to set return value '%s', return values can be returned only with assignment",v->name);
            yyerror(str_err);
            break;
        case ID_LOST:
            //ignore this error
            break;
        case ID_STRING:
        case ID_FUNC:
        case ID_PROC:
        case ID_FORWARDED_FUNC:
        case ID_FORWARDED_PROC:
        case ID_TYPEDEF:
        case ID_PROGRAM_NAME:
            yyerror("UNEXPECTED_ERROR: 44-44");
            exit(EXIT_FAILURE);
        }
        new_ir = link_stmt_to_stmt(new_ir,read_stmt);
    }
    return read_stmt;
}

ir_node_t *new_write_stmt(expr_list_t *list) {
    int i;
    int error=0;
    ir_node_t *new_ir;
    ir_node_t *write_stmt;
    ir_node_t *ir_result_lval;
    expr_t *l;
    data_t *d;

    //print every possible error
    for(i=0;i<MAX_EXPR_LIST-list->expr_list_empty;i++) {
        l = list->expr_list[i];
        if (!TYPE_IS_STANDARD(l->datatype) && l->expr_is!=EXPR_STRING && l->expr_is==EXPR_LVAL && !TYPE_IS_STRING(l->datatype)) {
            //take in mind the lvalue STRINGS too
            if (l->expr_is==EXPR_LVAL) {
                sprintf(str_err,"ERROR: write statement can only print standard types and strings, '%s' of type '%s'",l->var->name,l->var->datatype->data_name);
                yyerror(str_err);
            }
            else if (l->expr_is!=EXPR_LOST) {
                sprintf(str_err,"ERROR: write statement can only print standard types and strings");
                yyerror(str_err);
            }
            //else EXPR_LOST
            error = 1;
        }
    }

    if (error) {
        //including EXPR_LOST
        return NULL;
    }

    write_stmt = NULL;
    for(i=0;i<MAX_EXPR_LIST-list->expr_list_empty;i++) {
        new_ir = NULL;
        l = list->expr_list[i];
        if (l->expr_is==EXPR_STRING || (l->expr_is==EXPR_LVAL && TYPE_IS_STRING(l->datatype))) {
            //STRING
            //STRINGS only have one dimension
            new_ir = new_ir_node_t(NODE_OUTPUT_STRING);
            new_ir->ir_lval = calculate_lvalue(l->var);
        }
        else if (l->expr_is==EXPR_RVAL || l->expr_is==EXPR_HARDCODED_CONST || l->expr_is==EXPR_LVAL) {
            d = l->datatype;

            ir_result_lval = NULL;
            if (l->expr_is==EXPR_LVAL) {
                ir_result_lval = calculate_lvalue(l->var);
            }

            if (d->is==TYPE_INT) {
                new_ir = new_ir_node_t(NODE_OUTPUT_INT);
                new_ir->ir_lval = ir_result_lval;
                new_ir->ir_rval = expr_tree_to_ir_tree(l);
            }
            else if (d->is==TYPE_REAL) {
                new_ir = new_ir_node_t(NODE_OUTPUT_REAL);
                new_ir->ir_lval = ir_result_lval;
                new_ir->ir_rval = expr_tree_to_ir_tree(l);
            }
            else if (d->is==TYPE_BOOLEAN) {
                //read int and check the value
                new_ir = new_ir_node_t(NODE_OUTPUT_BOOLEAN);
                new_ir->ir_lval = ir_result_lval;
                new_ir->ir_rval = expr_tree_to_ir_tree(l);
            }
            else if (d->is==TYPE_CHAR) {
                new_ir = new_ir_node_t(NODE_OUTPUT_CHAR);
                new_ir->ir_lval = ir_result_lval;
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
