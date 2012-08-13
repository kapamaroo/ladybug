#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "build_flags.h"
#include "statements.h"
#include "expr_toolbox.h"

void opt_for_init() {}

void opt_move_independent_statements_out_of_loop(statement_t *stmt) {
    statement_t *curr;
    statement_t *head;
    statement_t *new_s;
    var_t *locked_var;

    die("UNSAFE_CALL: still missing basic checks for correct code generation");

    if (stmt->type != ST_For)
        die("INTERNAL_ERROR: expected for_stmt");

    curr = stmt->_for.loop;
    if (curr->type != ST_Comp)
        die("INTERNAL_ERROR: expected comp_stmt");

    new_s = NULL;
    curr = curr->_comp.first_stmt;
    head = curr;
    locked_var = stmt->_for.var;

    while (curr) {
        //reminder: cannot write directly to locked var,
        //          it is read only for the body statements

        if (!VAR_IS_GUARDED_ARRAY_ELEMENT(locked_var,curr->_assignment.var) &&
            !EXPR_CONTAINS_GUARD(locked_var,curr->_assignment.expr)) {
            head = unlink_statement(curr,head);
            new_s = link_statements(curr,new_s);
        }

        curr = curr->next;
    }

    //update loop pointer
    stmt->_for.loop->_comp.first_stmt = head;

    //wrap prologue into comp_stmt
    new_s = statement_comp(new_s);

    stmt->_for.prologue = new_s;
}

//TODO
//////////////////////////////////////////////////////////////////////

void break_DEP_WAR(dep_vector_t *dep_vector, dep_t *dep) {}

dep_t *detect_unbreakable_dep_route(dep_vector_t *dep_vector) {}

dep_t *detect_breakable_dep_route(dep_vector_t *dep_vector) {}

int break_dep_route(dep_vector_t *dep_vector, dep_t *dep) {}

//////////////////////////////////////////////////////////////////////

void shift_lvalue(var_t *v, int copy_num) {
    if (v->from_comp &&
        v->from_comp->comp_type == TYPE_ARRAY)
        v->from_comp->array.unroll_offset = copy_num;
}

void shift_all_expr_lvalues(expr_t *ltree, int copy_num) {
    if (!ltree)
        return;

    if (ltree->expr_is == EXPR_LVAL)
        shift_lvalue(ltree->var,copy_num);
    else {
        shift_all_expr_lvalues(ltree->l1,copy_num);
        shift_all_expr_lvalues(ltree->l2,copy_num);
    }
}

void shift_all_stmt_lvalues(statement_t *s, int copy_num) {
    if (s->type != ST_Assignment)
        die("NOT_IMPLEMENTED: expected assignment");

    var_t *v = s->_assignment.var;
    expr_t *l = s->_assignment.expr;

    shift_lvalue(v,copy_num);
    shift_all_expr_lvalues(l,copy_num);
}

void shift_all_stmt_list_lvalues(statement_t *head, int known) {
    statement_t *curr = head;

    while (curr) {
        shift_all_stmt_lvalues(curr,known);
        curr = curr->next;
    }
}

var_t *deep_copy_var(var_t *v) {
    var_t *new_v;
    info_comp_t *new_info;

    if (!v)
        return NULL;

    new_v = (var_t*)calloc(1,sizeof(var_t));
    new_v = memcpy(new_v,v,sizeof(var_t));

    if (v->from_comp) {
        new_info = (info_comp_t*)calloc(1,sizeof(info_comp_t));
        new_info = memcpy(new_info,v->from_comp,sizeof(info_comp_t));
        new_v->from_comp = new_info;
    }

    new_v->to_expr = expr_version_of_variable(new_v);

    return new_v;
}

expr_t *deep_copy_expr_tree(expr_t *ltree) {
    expr_t *new_ltree;

    if (!ltree)
        return NULL;

    new_ltree = (expr_t*)calloc(1,sizeof(expr_t));
    new_ltree = memcpy(new_ltree,ltree,sizeof(expr_t));

    if (ltree->expr_is == EXPR_LVAL)
        new_ltree->var = deep_copy_var(ltree->var);
    else {
        new_ltree->l1 = deep_copy_expr_tree(ltree->l1);
        new_ltree->l2 = deep_copy_expr_tree(ltree->l2);
    }

    return new_ltree;
}

statement_t *deep_copy_stmt(statement_t *s) {
    statement_t *new_s;

    if (!s)
        return NULL;

    if (s->type != ST_Assignment)
        die("INTERNAL_ERROR: deep_copy_stmt(): expected assignment");

    new_s = (statement_t*)calloc(1,sizeof(statement_t));
    new_s = memcpy(new_s,s,sizeof(statement_t));

    new_s->_assignment.var = deep_copy_var(s->_assignment.var);
    new_s->_assignment.expr = deep_copy_expr_tree(s->_assignment.expr);

    return new_s;
}

statement_t *deep_copy_stmt_list(statement_t *head) {
    statement_t *curr = head;
    statement_t *replica = NULL;

    while (curr) {
        statement_t *new_s = deep_copy_stmt(curr);
        replica = link_statements(new_s,replica);
        curr = curr->next;
    }

    return replica;
}

void unroll_loop_body(statement_t *body, int times) {
    int i;
    statement_t *new_s;
    statement_t *head;
    statement_t *new_head;

    if (body->type != ST_For)
        die("INTERNAL_ERROR: loop_unroll: expected for_stmt");

    int iter_start = body->_for.iter->start->ival;
    int iter_stop = body->_for.iter->stop->ival;
    int leftovers = (iter_stop - iter_start) % times;

    if (leftovers) {
        //update iter range
        iter_stop -= leftovers;
        body->_for.iter->stop.ival = iter_stop;

        new_head = NULL;
        for (i=1; i<=leftovers; i++) {
            int known = iter_stop + i;
            new_head = deep_copy_stmt_list(head);
            shift_all_stmt_list_lvalues(new_head,known);
        }

        //link leftover iterations to epilogue
        body->_for.epilogue = link_statements(new_head,body->_for.epilogue);
    }

    //update iter, multiply step by times
    body->_for.iter->step->ival *= times;

    new_head = NULL;
    head = body->_for.loop->_comp.first_stmt;

    for (i=1; i<=times; i++) {
        new_head = deep_copy_stmt_list(head);
        shift_all_stmt_list_lvalues(new_head,i);
    }

    //link all copys to original body loop
    new_head = link_statements(new_head,head);

    body->_for.loop->_comp.first_stmt = new_head;
}

void replace_var_with_hardcoded_int_in_stmt(statement_t *s, var_t *v, int known) {
    if (s->type != ST_Assignment)
        die("NOT_IMPLEMENTED: expected assignment");

    if (s->_assignment.var == v)
        die("UNEXPECTED_ERROR: replacing var in assignment with hardcoded value");

    s->_assignment.expr = expr_replace_var_with_hardcoded_int(s->_assignment.expr,v,known);
}

statement_t *gen_wrapper_for_sym_unroll(statement_t *head, var_t *guard, int bsize, iter_t *iter, int gen_prologue) {
    //see algorithms/sym_unroll_wrapper.c

    int i;
    int j;

    statement_t *wrapper = NULL;
    statement_t *tmp;

    //create tmp hardcoded_value
    int known = (gen_prologue) ? iter->start->ival : iter->stop->ival;

    for (i=0; i<bsize-1; i++) {
        //controls the loop shift offset
        int copy_num = (gen_prologue) ? i : bsize - i - 1;

        statement_t *curr = head;
        statement_t *new_p = NULL;

        //make prologe for i-th iteration
        for (j=0; j<bsize-i-1; j++) {
            tmp = deep_copy_stmt(curr);
            shift_all_stmt_lvalues(tmp,copy_num);
            //guard var is known, replace it
            replace_var_with_hardcoded_int_in_stmt(tmp,guard,known);

            new_p = link_statements(tmp,new_p);
            curr = curr->next;
        }

        if (gen_prologue)
            wrapper = link_statements(new_p,wrapper);
        else
            //epilogue, reverse linking in respect of mem access pattern
            wrapper = link_statements(wrapper,new_p);

        //update known value of guard var
        ++known;
    }

    wrapper = statement_comp(wrapper);

    return wrapper;
}

void unroll_loop_symbolic(statement_t *body) {
#define GEN_PROLOGUE 1
#define GEN_EPILOGUE 0

    if (body->type != ST_For)
        die("INTERNAL_ERROR: symbolic_unroll: expected for_stmt");

    int i;
    int bsize = body->size;

    //update iter to avoid out of bounds code generation
    //see the prologue/epilogue generator for more info
    body->_for.iter->stop->ival -= bsize - 1;

    statement_t *head = body->_for.loop->_comp.first_stmt;
    var_t *guard = body->_for.var;
    iter_t *iter = body->_for.iter;

    statement_t *prologue = gen_wrapper_for_sym_unroll(head,guard,bsize,iter,GEN_PROLOGUE);
    statement_t *epilogue = gen_wrapper_for_sym_unroll(head,guard,bsize,iter,GEN_EPILOGUE);

    //now it is safe to change the original loop
    statement_t *curr = head;
    for (i=0; i<bsize-1; i++) {
        shift_all_stmt_lvalues(curr,bsize-1-i);
        curr = curr->next;
    }

    //prologue/epilogue may not be empty
    body->_for.prologue = link_statements(prologue,body->_for.prologue);
    body->_for.epilogue = link_statements(epilogue,body->_for.epilogue);
}
