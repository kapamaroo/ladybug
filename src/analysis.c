#include <stdio.h>
#include <stdlib.h>

#include "build_flags.h"
#include "analysis.h"
#include "statements.h"
#include "expr_toolbox.h"

void do_analyse_block(statement_t *block);
var_list_t *do_expr_list_analysis(var_list_t *var_list, expr_list_t *expr_list);

statement_t *do_blocks_in_module(statement_t *root) {
    statement_t *pending;
    statement_t *new_block;
    statement_t *next_block;
    statement_t *new_root;
    statement_t *current;

    //set initial start point
    //reminder: root statement of modules are comp_statements
    pending = root->_comp.first_stmt;
    current = NULL;

    if (NEW_STMT_BLOCK_STARTS_FROM(pending)) {
        new_root = pending;
        pending = pending->next;
        while (pending && NEW_STMT_BLOCK_STARTS_FROM(pending)) pending = pending->next;
        if (pending) {
            //break from new_root
            pending->prev->next = NULL;
            current = pending->next;
        }
    } else {
        new_root = NULL;
        current = pending->next;
    }

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

var_list_t *insert_to_var_list(var_list_t *var_list, var_t *var) {
    int i;

    //do not insert if already in the list
    if (var_list) {
        for (i=0; i<var_list->all_var_num; i++) {
            if (var_list->var_list[i]==var) {
                return var_list;
            }
        }
    }

    var_list = var_list_add(var_list,var);
    return var_list;
}

var_list_t *merge_var_lists(var_list_t *dest, var_list_t *src) {
    int i;

    if (src)
        for (i=0; i<src->all_var_num; i++) {
            dest = insert_to_var_list(dest, src->var_list[i]);
        }

    return dest;
}

void merge_stmt_analysis_to_block(statement_t *stmt, statement_t *block) {
    block->stats_of.block.read =
        merge_var_lists(
                        block->stats_of.block.read,
                        stmt->stats_of.block.read);

    block->stats_of.block.write =
        merge_var_lists(
                        block->stats_of.block.write,
                        stmt->stats_of.block.write);
}

var_list_t *do_expr_tree_analysis(var_list_t *var_list, expr_t *l) {
    //reminder: we are error free
    switch (l->expr_is) {
    case EXPR_RVAL:
        var_list = do_expr_tree_analysis(var_list,l->l1);
        var_list = do_expr_tree_analysis(var_list,l->l2);
        break;
    case EXPR_LVAL:
#warning ignore ID_RETURN in analysis //FIXME
        if (l->var->id_is==ID_RETURN) {
            //return var_list;
            //why is this working? see expr_toolbox.c: expression_from_function_call()
            var_list = do_expr_list_analysis(var_list,l->expr_list);
        } else {
            var_list = insert_to_var_list(var_list,l->var);
        }
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
    s->stats_of.block.write =
        var_list_add(s->stats_of.block.write,s->_assignment.var);

    s->stats_of.block.read =
        do_expr_tree_analysis(s->stats_of.block.read,s->_assignment.expr);
}

void do_call_analysis(statement_t *s) {
    //reminder: this is a procedure call, no write dependencies

    s->stats_of.block.read =
        do_expr_list_analysis(s->stats_of.block.read,s->_call.expr_params);

}

void do_read_analysis(statement_t *s) {
    //read stmt has only write dependencies

    //variable list is ready, see bison.y
    s->stats_of.block.write = s->_read.var_list;
}

void do_write_analysis(statement_t *s) {
    //write stmt has only read dependencies

    s->stats_of.block.read =
        do_expr_list_analysis(s->stats_of.block.read,s->_write.expr_list);

}

void do_analyse_stmt(statement_t *s,statement_t *block) {
#if (BISON_DEBUG_LEVEL >=1)
    int i=0;
    int j=0;
#endif

    while (s) {
        switch (s->type) {
            //block statements
        case ST_If:          do_analyse_block(s);    break;
        case ST_While:       do_analyse_block(s);    break;
        case ST_For:         do_analyse_block(s);    break;
        case ST_Comp:        do_analyse_block(s);    break;

            //primitive statements
        case ST_Call:        do_call_analysis(s);    break;
        case ST_Assignment:  do_assign_analysis(s);  break;
        case ST_Read:        do_read_analysis(s);    break;
        case ST_Write:       do_write_analysis(s);   break;

        case ST_With:          /* virtual stmt */
        case ST_BadStatement:  /* ignore */          break;
        default:
            die("UNEXPECTED_ERROR: expected statement for analysis");
        }

        //merge analysis
        merge_stmt_analysis_to_block(s,block);

        block->stats_of.block.size++;
        /*
#if (BISON_DEBUG_LEVEL >=1)
        printf("debug:\t\t\tstmt %d:",i);
        if (s->stats_of.block.write) {
            printf("\t\tOUT_VECTOR(");
            for (j=0;j<s->stats_of.block.write->all_var_num;j++) {
                printf("%s,",s->stats_of.block.write->var_list[j]->name);
            }
            printf(")");
        } else {
            //align output
            printf("\t\t\t");
        }

        if (s->stats_of.block.read) {
            printf("\t\tIN_VECTOR(");
            for (j=0;j<s->stats_of.block.read->all_var_num;j++) {
                printf("%s,",s->stats_of.block.read->var_list[j]->name);
            }
            printf(")");
        }

        printf("\n");
        i++;
        #endif
*/
        s = s->next;
    }
}

void do_analyse_block(statement_t *block) {
    static int nesting = -1;

    nesting++;

    switch (block->type) {
    case ST_If:
        block->stats_of.block.read =
            do_expr_tree_analysis(
                                  block->stats_of.block.read,
                                  block->_if.condition);

        do_analyse_stmt(block->_if._true,block->_if._true);
        do_analyse_stmt(block->_if._false,block->_if._false);

        merge_stmt_analysis_to_block(block->_if._true,block);
        merge_stmt_analysis_to_block(block->_if._false,block);

        break;
    case ST_While:
        block->stats_of.block.read =
            do_expr_tree_analysis(
                                  block->stats_of.block.read,
                                  block->_while.condition);

        do_analyse_stmt(block->_while.loop,block);       break;
    case ST_For:
        do_analyse_stmt(block->_for.loop,block);         break;
    case ST_Comp:
        do_analyse_stmt(block->_comp.first_stmt,block);  break;
    default:
        die("UNEXPECTED_ERROR: expected block for analysis");
    }

    block->stats_of.block.depth = nesting;

    nesting--;

#if (BISON_DEBUG_LEVEL >=1)
    printf("debug:\tblock-nesting: %d <==> statements %d\t",block->stats_of.block.depth,block->stats_of.block.size);

    int j;
    //printf("debug:\t\t\tblock:");
    if (block->stats_of.block.write) {
        printf("\t\tOUT_VECTOR(");
        for (j=0;j<block->stats_of.block.write->all_var_num;j++) {
            printf("%s,",block->stats_of.block.write->var_list[j]->name);
        }
        printf(")");
    } else {
        //align output
        printf("\t\t\t");
    }

    if (block->stats_of.block.read) {
        printf("\t\tIN_VECTOR(");
        for (j=0;j<block->stats_of.block.read->all_var_num;j++) {
            printf("%s,",block->stats_of.block.read->var_list[j]->name);
        }
        printf(")");
    }
    printf("\n");
#endif
}

void analyse_blocks() {
    int i;
    statement_t *current;

    //current refers to a block

    for (i=0;i<statement_root_module_current_free;i++) {
        current = statement_root_module[i];
        printf("debug:\ndebug:\t*** MODULE %d ***\ndebug:\n",i);
        while (current) {
            if (!NEW_STMT_BLOCK_STARTS_FROM(current)) {
                die("UNEXPECTED_ERROR: expected block, bad block generator");
            }
            do_analyse_block(current);
            current = current->next;
        }
    }
}
