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
            //return var_list;
            //why is this working? see expr_toolbox.c: expression_from_function_call()
            var_list = do_expr_list_analysis(var_list,l->expr_list);
        } else {
            //do not insert if already in the list
            for (i=0;(var_list && i<var_list->all_var_num);i++) {
                if (var_list->var_list[i]==l->var) {
                    return var_list;
                }
            }
            var_list = var_list_add(var_list,l->var);
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
    s->stats_of.stmt.write =
        var_list_add(s->stats_of.stmt.write,s->_assignment.var);

    s->stats_of.stmt.read =
        do_expr_tree_analysis(s->stats_of.stmt.read,s->_assignment.expr);
}

void do_call_analysis(statement_t *s) {
    //reminder: this is a procedure call, no write dependencies

    s->stats_of.stmt.read =
        do_expr_list_analysis(s->stats_of.stmt.read,s->_call.expr_params);

}

void do_read_analysis(statement_t *s) {
    //read stmt has only write dependencies

    //variable list is ready, see bison.y
    s->stats_of.stmt.write = s->_read.var_list;
}

void do_write_analysis(statement_t *s) {
    //write stmt has only read dependencies

    s->stats_of.stmt.read =
        do_expr_list_analysis(s->stats_of.stmt.read,s->_write.expr_list);

}

inline void merge_stmt_analysis_to_block(statement_t *s, statement_t *block) {
    int i;
    int size;

    size = block->stats_of.block.size;
    //printf("debug: initial size: %d\n",size);

    switch (s->type) {
    case ST_If:
        size += s->_if._true->stats_of.block.size;
        if (s->_if._false) { size += s->_if._false->stats_of.block.size; }
        //printf("debug: line %d\tsize: %d\n",__LINE__,size);
        if (size>MAX_BLOCK_SIZE) { die("UNIMPLEMENTED: cannot handle block size"); }

        for (i=0;i<s->_if._true->stats_of.block.size;i++) {
            s->stats_of.block.dependencies[s->stats_of.block.size++] =
                s->_if._true->stats_of.block.dependencies[i];
        }

        if (s->_if._false) {
            for (i=0;i<s->_if._false->stats_of.block.size;i++) {
                s->stats_of.block.dependencies[s->stats_of.block.size++] =
                    s->_if._false->stats_of.block.dependencies[i];
            }
        }
        break;
    case ST_While:
        size += s->_while.loop->stats_of.block.size;
        //printf("debug: line %d\tsize: %d\n",__LINE__,size);
        if (size>MAX_BLOCK_SIZE) { die("UNIMPLEMENTED: cannot handle block size"); }

        for (i=0;i<s->_while.loop->stats_of.block.size;i++) {
            s->stats_of.block.dependencies[s->stats_of.block.size++] =
                s->_while.loop->stats_of.block.dependencies[i];
        }
        break;
    case ST_For:
        size += s->_while.loop->stats_of.block.size;
        printf("debug: line %d\tsize: %d\n",__LINE__,size);
        if (size>MAX_BLOCK_SIZE) { die("UNIMPLEMENTED: cannot handle block size"); }

        for (i=0;i<s->_for.loop->stats_of.block.size;i++) {
            s->stats_of.block.dependencies[s->stats_of.block.size++] =
                s->_for.loop->stats_of.block.dependencies[i];
        }
        break;
    case ST_Comp:
        size += s->stats_of.block.size;
        //printf("debug: line %d\tsize: %d\n",__LINE__,size);
        if (size>MAX_BLOCK_SIZE) { die("UNIMPLEMENTED: cannot handle block size"); }

        for (i=0;i<s->stats_of.block.size;i++) {
            block->stats_of.block.dependencies[block->stats_of.block.size++] =
                s->stats_of.block.dependencies[i];
        }
        break;
    default:
        //printf("debug: line %d\tsize: %d\n",__LINE__,size);
        if (size>MAX_BLOCK_SIZE) { die("UNIMPLEMENTED: cannot handle block size"); }

        block->stats_of.block.dependencies[block->stats_of.block.size++] = &s->stats_of.stmt;
    }
}

void do_analyse_stmt(statement_t *s,statement_t *block) {
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
        merge_stmt_analysis_to_block(s,block);
        s = s->next;
    }
}

void do_analyse_block(statement_t *block) {
    static int nesting = -1;

    nesting++;

    switch (block->type) {
    case ST_If:            do_analyse_stmt(block->_if._true,block);
                           do_analyse_stmt(block->_if._false,block);        break;
    case ST_While:         do_analyse_stmt(block->_while.loop,block);       break;
    case ST_For:           do_analyse_stmt(block->_for.loop,block);         break;
    case ST_Comp:          do_analyse_stmt(block->_comp.first_stmt,block);  break;
    default:
        die("UNEXPECTED_ERROR: expected block for analysis");
    }

    block->stats_of.block.depth = nesting;

    nesting--;

#if (BISON_DEBUG_LEVEL >=1)
    printf("debug:\tblock-nesting: %d <==> statements %d\t\n",block->stats_of.block.depth,block->stats_of.block.size);

    int i,j;
    for (i=0;i<block->stats_of.block.size;i++) {
        printf("debug:\t\t\tstmt %d:",i);
        if (block->stats_of.block.dependencies[i]->write) {
            printf("\t\tOUT_VECTOR(");
            for (j=0;j<block->stats_of.block.dependencies[i]->write->all_var_num;j++) {
                printf("%s,",block->stats_of.block.dependencies[i]->write->var_list[j]->name);
            }
            printf(")");
        } else {
            //align output
            printf("\t\t\t");
        }

        if (block->stats_of.block.dependencies[i]->read) {
            printf("\t\tIN_VECTOR(");
            for (j=0;j<block->stats_of.block.dependencies[i]->read->all_var_num;j++) {
                printf("%s,",block->stats_of.block.dependencies[i]->read->var_list[j]->name);
            }
            printf(")");
        }

        printf("\n");
    }
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
            if (current->type!=ST_Comp) {
                die("UNEXPECTED_ERROR: expected block");
            }
            do_analyse_block(current);
            current = current->next;
        }
    }
}
