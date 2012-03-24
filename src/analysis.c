#include <stdio.h>
#include <stdlib.h>

#include "analysis.h"
#include "statements.h"
#include "expr_toolbox.h"

statement_t *do_blocks_in_module(statement_t *root) {
    statement_t *pending;
    statement_t *new_block;
    statement_t *next_block;
    statement_t *new_root;
    statement_t *current;

    new_root = NULL;

    //set initial start point
    //reminder: root statement of modules are comp_statements
    current = root->_comp.first_stmt->next;
    pending = root->_comp.first_stmt;

    //we don't need the initial big block
    free(root);

    while (current) {
        if (NEW_STMT_BLOCK_STARTS_FROM(current)) {
            //break link
            current->prev->next = NULL;

            //make new block and link it to new_root
            new_block = statement_comp(pending);
            new_root = link_statements(new_block,new_root);
            new_root = link_statements(current,new_root);

            //set new pending point
            pending = current->next;
        }

        current = current->next;
    }

    if (pending) {
        //make final block
        new_block = statement_comp(pending);
        new_root = link_statements(new_block,new_root);
    }

    return new_root;
}

void define_blocks() {
    int i;
    statement_t *current;

    //current refers to a module

    for (i=0;i<MAX_NUM_OF_MODULES;i++) {
        current = statement_root_module[i];
        if (current) {
            statement_root_module[i] = do_blocks_in_module(current);
        }
    }
}

var_list_t *do_expr_tree_analysis(var_list_t *var_list, expr_t *l) {
    int i;

    //reminder: we are error free

    if (!l) {
        return;
    }

    switch (l->expr_is) {
    case EXPR_RVAL:
        var_list = do_expr_tree_analysis(var_list,l->l1);
        var_list = do_expr_tree_analysis(var_list,l->l2);
        break;
    case EXPR_LVAL:
#warning ignore ID_RETURN in analysis //FIXME
        if (l->var->id_is==ID_RETURN) {
            return var_list;
        }

        //do not insert if already in the list
        for (i=0;(var_list && i<var_list->all_var_num);i++) {
            if (var_list->var_list[i]==l->var) {
                return var_list;
            }
        }
        var_list = var_list_add(var_list,l->var);
        break;
    case EXPR_HARDCODED_CONST:
    case EXPR_STRING:
    case EXPR_SET:
    case EXPR_NULL_SET:
        break;
    case EXPR_LOST:
        die("INTERNAL_ERROR: lost variable still alive in analysis");
    }

    return var_list;
}

var_list_t *do_expr_list_analysis(var_list_t *var_list, expr_list_t *expr_list) {
    int i;
    int size;

    size = expr_list->all_expr_num;

    for (i=0;i<size;i++) {
        var_list =
            do_expr_tree_analysis(var_list,expr_list->expr_list[i]);
    }

    return var_list;
}

void do_assign_analysis(statement_t *s) {
    s->stats_of._assign.write =
        var_list_add(s->stats_of._assign.write,s->_assignment.var);

    s->stats_of._assign.read =
        do_expr_tree_analysis(s->stats_of._assign.read,s->_assignment.expr);
}

void do_call_analysis(statement_t *s) {
    //reminder: this is a procedure call, no write dependencies

    s->stats_of._call.read =
        do_expr_list_analysis(s->stats_of._call.read,s->_call.expr_params);

}

void do_read_analysis(statement_t *s) {
    //read stmt has only write dependencies

    //variable list is ready, see bison.y
    s->stats_of._read.write = s->_read.var_list;
}

void do_write_analysis(statement_t *s) {
    //write stmt has only read dependencies

    s->stats_of._write.read =
        do_expr_list_analysis(s->stats_of._write.read,s->_write.expr_list);

}

void do_loop_analysis(statement_t *first_stmt) {

}

void do_analyse_block(statement_t *block_root) {
    statement_t *s;

    s = block_root;

    while (s) {
        switch (s->type) {

            //block statements
        case ST_If:            do_analyse_block(s->_if._true);
            if (s->_if._false) { do_analyse_block(s->_if._false); }    break;
        case ST_While:         do_analyse_block(s->_while.loop);       break;
        case ST_For:           do_analyse_block(s->_for.loop);         break;
        case ST_Comp:          do_analyse_block(s->_comp.first_stmt);  break;

            //primitive statements
        case ST_Call:          do_call_analysis(s);                    break;
        case ST_Assignment:    do_assign_analysis(s);                  break;
        case ST_Read:          do_read_analysis(s);                    break;
        case ST_Write:         do_write_analysis(s);                   break;

        case ST_With:          /* virtual stmt, never generated */
        case ST_BadStatement:  /* ignore bad statement */              break;
        }

        s = s->next;
    }
}

void analyse_blocks() {
    int i;
    statement_t *current;

    //current refers to a block

    for (i=0;i<MAX_NUM_OF_MODULES;i++) {
        current = statement_root_module[i];
        while (current) {
            do_analyse_block(current);
            current = current->next;
        }
    }
}
