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

void break_DEP_WAR(dep_vector_t *dep_vector, dep_t *dep) {}

dep_t *detect_unbreakable_dep_route(dep_vector_t *dep_vector) {}

dep_t *detect_breakable_dep_route(dep_vector_t *dep_vector) {}

int break_dep_route(dep_vector_t *dep_vector, dep_t *dep) {}

//////////////////////////////////////////////////////////////////////

var_t *deep_copy_var(var_t *v, int copy_num) {
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

        if (v->from_comp->comp_type == TYPE_ARRAY)
            new_v->from_comp->array.unroll_offset = copy_num;
    }

    new_v->to_expr = expr_version_of_variable(new_v);

    return new_v;
}

expr_t *deep_copy_expr_tree(expr_t *ltree, int copy_num) {
    expr_t *new_ltree;

    if (!ltree)
        return NULL;

    new_ltree = (expr_t*)calloc(1,sizeof(expr_t));
    new_ltree = memcpy(new_ltree,ltree,sizeof(expr_t));

    if (ltree->var)
        new_ltree->var = deep_copy_var(ltree->var,copy_num);

    if (ltree->l1)
        new_ltree->l1 = deep_copy_expr_tree(ltree->l1,copy_num);

    if (ltree->l2)
        new_ltree->l2 = deep_copy_expr_tree(ltree->l2,copy_num);

    return new_ltree;
}

statement_t *deep_copy_stmt(statement_t *s, int copy_num) {
    statement_t *new_s;

    if (!s)
        return NULL;

    if (s->type != ST_Assignment)
        die("INTERNAL_ERROR: deep_copy_stmt(): expected assignment");

    new_s = (statement_t*)calloc(1,sizeof(statement_t));
    new_s = memcpy(new_s,s,sizeof(statement_t));

    new_s->_assignment.var = deep_copy_var(s->_assignment.var,copy_num);
    new_s->_assignment.expr = deep_copy_expr_tree(s->_assignment.expr,copy_num);

    return new_s;
}

void unroll_loop_body(statement_t *body, int times) {
    int i;
    statement_t *new_s;
    statement_t *curr;
    statement_t *head;
    statement_t *new_head;
    statement_t *replica;

    if (body->type != ST_For)
        die("INTERNAL_ERROR: loop_unroll: expected for_stmt");

    new_head = NULL;
    head = body->_for.loop->_comp.first_stmt;

    for (i=1; i<=times; i++) {
        replica = NULL;
        curr = head;

        while (curr) {
            new_s = deep_copy_stmt(curr,i);
            replica = link_statements(new_s,replica);
            curr = curr->next;
        }

        new_head = link_statements(replica,new_head);
    }

    //link all replicas to original body loop
    new_head = link_statements(new_head,head);

    body->_for.loop->_comp.first_stmt = new_head;
}

statement_t *gen_wrapper_for_sym_unroll(statement_t *head, int bsize, int gen_prologue) {
    //see algorithms/sym_unroll_wrapper.c

    int i;
    int j;
    statement_t *wrapper = NULL;

    for (i=0; i<bsize-1; i++) {
        int copy_num = (gen_prologue) ? i : bsize - i - 1;

        statement_t *curr = head;
        statement_t *new_p = NULL;

        //make prologe for i-th iteration
        for (j=0; j<bsize-i-1; j++) {
            statement_t *tmp = deep_copy_stmt(curr,copy_num);
#warning must replace guard var with hardcoded value 'copy_num'
            new_p = link_statements(tmp,new_p);
            curr = curr->next;
        }

        if (gen_prologue)
            wrapper = link_statements(new_p,wrapper);
        else
            //epilogue, reverse linking in respect of mem access pattern
            wrapper = link_statements(wrapper,new_p);
    }

    wrapper = statement_comp(wrapper);

    return wrapper;
}

void unroll_loop_symbolic(statement_t *body) {
#define GEN_PROLOGUE 1
#define GEN_EPILOGUE 0

    if (body->type != ST_For)
        die("INTERNAL_ERROR: symbolic_unroll: expected for_stmt");

    int bsize = body->size;

    statement_t *head = body->_for.loop->_comp.first_stmt;
    statement_t *prologue = gen_wrapper_for_sym_unroll(head,bsize,GEN_PROLOGUE);
    statement_t *epilogue = gen_wrapper_for_sym_unroll(head,bsize,GEN_EPILOGUE);

    //prologue may not be empty
    body->_for.prologue = link_statements(prologue,body->_for.prologue);

    //epilogue may not be empty
    body->_for.epilogue = link_statements(epilogue,body->_for.epilogue);
}
