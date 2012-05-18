#include <stdio.h>
#include <stdlib.h>

#include "build_flags.h"
#include "analysis.h"
#include "statements.h"
#include "expr_toolbox.h"

void do_analyse_block(statement_t *block);
var_list_t *do_expr_list_analysis(var_list_t *var_list, expr_list_t *expr_list);

void dependence_analysis(statement_t *block);
dep_vector_t *do_dependence_analysis(statement_t *comp_stmt);

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
    block->stats_of_vars.read =
        merge_var_lists(
                        block->stats_of_vars.read,
                        stmt->stats_of_vars.read);

    block->stats_of_vars.write =
        merge_var_lists(
                        block->stats_of_vars.write,
                        stmt->stats_of_vars.write);
}

var_list_t *do_expr_tree_analysis(var_list_t *var_list, expr_t *l) {
    //reminder: we are error free

    if (!l) {
        //some operators have only one child
        return var_list;
    }

    switch (l->expr_is) {
    case EXPR_RVAL:
        var_list = do_expr_tree_analysis(var_list,l->l1);
        var_list = do_expr_tree_analysis(var_list,l->l2);
        break;
    case EXPR_LVAL:
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
    s->stats_of_vars.write =
        var_list_add(s->stats_of_vars.write,s->_assignment.var);

    s->stats_of_vars.read =
        do_expr_tree_analysis(s->stats_of_vars.read,s->_assignment.expr);
}

void do_call_analysis(statement_t *s) {
    //reminder: this is a procedure call, no write dependencies

    s->stats_of_vars.read =
        do_expr_list_analysis(s->stats_of_vars.read,s->_call.expr_params);

}

void do_read_analysis(statement_t *s) {
    //read stmt has only write dependencies

    //variable list is ready, see bison.y
    s->stats_of_vars.write = s->_read.var_list;
}

void do_write_analysis(statement_t *s) {
    //write stmt has only read dependencies

    s->stats_of_vars.read =
        do_expr_list_analysis(s->stats_of_vars.read,s->_write.expr_list);

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

        block->stats_of_vars.size++;
        /*
#if (BISON_DEBUG_LEVEL >=1)
        printf("debug:\t\t\tstmt %d:",i);
        if (s->stats_of_vars.write) {
            printf("\t\tOUT_VECTOR(");
            for (j=0;j<s->stats_of_vars.write->all_var_num;j++) {
                printf("%s,",s->stats_of_vars.write->var_list[j]->name);
            }
            printf(")");
        } else {
            //align output
            printf("\t\t\t");
        }

        if (s->stats_of_vars.read) {
            printf("\t\tIN_VECTOR(");
            for (j=0;j<s->stats_of_vars.read->all_var_num;j++) {
                printf("%s,",s->stats_of_vars.read->var_list[j]->name);
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

    /* consider the if statement as single statement inside another comp statement
       consider true and false statements as comp statements
     */


    switch (block->type) {
    case ST_If:
        block->stats_of_vars.read =
            do_expr_tree_analysis(
                                  block->stats_of_vars.read,
                                  block->_if.condition);

        do_analyse_stmt(block->_if._true,block->_if._true);
        do_analyse_stmt(block->_if._false,block->_if._false);

        merge_stmt_analysis_to_block(block->_if._true,block);
        merge_stmt_analysis_to_block(block->_if._false,block);
        break;
    case ST_While:
        block->stats_of_vars.read =
            do_expr_tree_analysis(
                                  block->stats_of_vars.read,
                                  block->_while.condition);

        do_analyse_stmt(block->_while.loop,block);
        break;
    case ST_For:
        do_analyse_stmt(block->_for.loop,block);
        break;
    case ST_Comp:
        do_analyse_stmt(block->_comp.first_stmt,block);
        break;
    default:
        die("UNEXPECTED_ERROR: expevcted block for analysis");
    }

    block->stats_of_vars.depth = nesting;

    nesting--;

#if (BISON_DEBUG_LEVEL >=1)
    printf("debug:\tblock-nesting: %d <==> statements %d\t",block->stats_of_vars.depth,block->stats_of_vars.size);

    int j;
    //printf("debug:\t\t\tblock:");
    if (block->stats_of_vars.write) {
        printf("\t\tOUT_VECTOR(");
        for (j=0;j<block->stats_of_vars.write->all_var_num;j++) {
            printf("%s,",block->stats_of_vars.write->var_list[j]->name);
        }
        printf(")");
    } else {
        //align output
        printf("\t\t\t");
    }

    if (block->stats_of_vars.read) {
        printf("\t\tIN_VECTOR(");
        for (j=0;j<block->stats_of_vars.read->all_var_num;j++) {
            printf("%s,",block->stats_of_vars.read->var_list[j]->name);
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
            dependence_analysis(current);

            current = current->next;
        }
    }
}

int EXPR_CONTAINS_GUARD(var_t *guard, expr_t *l) {

    if (!l) {
        //some operators have only one child
        return 0;
    }

    switch (l->expr_is) {
    case EXPR_RVAL:
        if (EXPR_CONTAINS_GUARD(guard,l->l1))
            return 1;
        if (EXPR_CONTAINS_GUARD(guard,l->l2))
            return 1;
        break;
    case EXPR_LVAL:
        if (l->var->id_is==ID_RETURN) {
            die("UNEXPECTED_ERROR: unrolling analysis of invalid for loop");
        } else {
            if (l->var==guard) {
                return 1;
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

inline expr_t *VAR_GUARD_CONTROLS_ARRAY_ELEMENT_INDEX(var_t *guard, expr_list_t *index) {
    int i;

    for (i=0; i<index->all_expr_num; i++) {
        if (EXPR_CONTAINS_GUARD(guard,index->expr_list[i])) {
            return index->expr_list[i];
        }
    }

    return NULL;
}

inline var_t *VAR_IS_GUARDED_ARRAY_ELEMENT(dep_vector_t *dep, var_t *v, int from) {
    info_comp_t *var_of;

    var_of = v->from_comp;

    while (var_of) {
        if (var_of->comp_type==TYPE_ARRAY) {
            expr_t *conflict_index = VAR_GUARD_CONTROLS_ARRAY_ELEMENT_INDEX(dep->guard,var_of->array.index);
            if (conflict_index) {
                if (from) {
                    dep->pool[dep->next_free_spot].conflict_index_from = conflict_index;
                } else {
                    dep->pool[dep->next_free_spot].conflict_index_to = conflict_index;
                }

                return var_of->array.base;
            }

            var_of = var_of->array.base->from_comp;
        }
        else {
            //else is record
            var_of = var_of->record.base->from_comp;
        }
    }

    return NULL;
}

inline void GUARDED_VARS_OF_THE_SAME_VAR_ARRAY(dep_vector_t *dep, var_t *from_var, var_t *to_var) {
    var_t *var_of1;
    var_t *var_of2;

    var_of1 = VAR_IS_GUARDED_ARRAY_ELEMENT(dep,from_var,1);
    var_of2 = VAR_IS_GUARDED_ARRAY_ELEMENT(dep,to_var,0);

    if (var_of1 && var_of1 == var_of2) {
        dep->pool[dep->next_free_spot].conflict_var = var_of1;
    }
}

void find_dependencies(dep_vector_t *dep, statement_t *from, statement_t *to, enum dependence_type dep_type) {
    int i;
    int j;

    var_list_t *from_list;
    var_list_t *to_list;

    switch (dep_type) {
    case DEP_RAR:
        to_list = to->stats_of_vars.read;
        from_list = from->stats_of_vars.read;
        break;
    case DEP_RAW:
        to_list = to->stats_of_vars.read;
        from_list = from->stats_of_vars.write;
        break;
    case DEP_WAR:
        to_list = to->stats_of_vars.write;
        from_list = from->stats_of_vars.read;
        break;
    case DEP_WAW:
        to_list = to->stats_of_vars.write;
        from_list = from->stats_of_vars.write;
        break;
    }

    //no dependencies
    if (!from_list || !to_list) {
        return;
    }

    for (i=0; i<from_list->all_var_num; i++) {
        for (j=0; j<to_list->all_var_num; j++) {
            if (from_list->var_list[i] == to_list->var_list[i]) {
                if (dep->guard) {
                    //for loop analysis
                    //extra info
                    GUARDED_VARS_OF_THE_SAME_VAR_ARRAY(dep,
                                                       from_list->var_list[i],
                                                       to_list->var_list[j]);
                }

                //commit dependence
                dep->pool[dep->next_free_spot].is = dep_type;
                dep->pool[dep->next_free_spot].from = from;
                dep->pool[dep->next_free_spot].to = to;
                dep->next_free_spot++;
            }
        }
    }
}

int estimate_num_of_dependencies(statement_t *current) {
    //upper bound

    int counter = 0;

    while (current) {
        if (current->stats_of_vars.write)
            counter += current->stats_of_vars.write->all_var_num;

        if (current->stats_of_vars.read)
            counter += current->stats_of_vars.read->all_var_num;

        current = current->next;
    }

    return counter;
}

dep_vector_t *do_dependence_analysis(statement_t *comp_stmt) {
    int estimate;
    dep_vector_t *dep;
    statement_t *from;
    statement_t *to;

    estimate = estimate_num_of_dependencies(comp_stmt->_comp.first_stmt);

    dep = (dep_vector_t*)calloc(1,sizeof(dep_vector_t));
    dep->pool = (dep_t*)calloc(estimate,sizeof(dep_t));

    from = comp_stmt->_comp.first_stmt;
    while (from) {

        to = from;
        find_dependencies(dep,from,to,DEP_WAR);
        to = to->next;

        while (to) {
            find_dependencies(dep,from,to,DEP_RAR);
            find_dependencies(dep,from,to,DEP_RAW);
            find_dependencies(dep,from,to,DEP_WAR);
            find_dependencies(dep,from,to,DEP_WAW);
            to = to->next;
        }

        from = from->next;
    }

    return dep;

}

int can_unroll_this_for_stmt(statement_t *block) {
    int i;
    statement_t *current;

    current = block->_for.loop->_comp.first_stmt;

    while (current) {
        //accept only primitive assignment statements
        if (current->type != ST_Assignment)
            return 0;

        //for statement must not contain function calls
        for (i=0; i<current->stats_of_vars.read->all_var_num; i++) {
            var_t *v = current->stats_of_vars.read->var_list[i];
            if (v->id_is==ID_RETURN) {
                return 0;
            }

            if (v->from_comp && v->from_comp->comp_type==TYPE_ARRAY) {
                //no multiple dimension arrays
                //if (v->from_comp->array->index->all_expr_num > 1) {
                if (v->from_comp->array.base->datatype->field_num > 1) {
                    return 0;
                }

                //simple enough index expression
                expr_t *l = v->from_comp->array.index->expr_list[0];

                if (l->expr_is == EXPR_RVAL && (l->op == OP_PLUS || l->op == OP_MINUS)) {
                    if (l->l1->expr_is == EXPR_LVAL && l->l2->expr_is != EXPR_HARDCODED_CONST) return 0;
                    if (l->l2->expr_is == EXPR_LVAL && l->l1->expr_is != EXPR_HARDCODED_CONST) return 0;
                } else if (l->expr_is != EXPR_LVAL) {
                    return 0;
                }
            }
        }

        for (i=0; i<current->stats_of_vars.write->all_var_num; i++) {
            var_t *v = current->stats_of_vars.write->var_list[i];
            if (v->from_comp && v->from_comp->comp_type==TYPE_ARRAY) {
                //no multiple dimension arrays
                if (v->from_comp->array.index->all_expr_num > 1) {
                    return 0;
                }

                //simple enough index expression
                expr_t *l = v->from_comp->array.index->expr_list[0];

                if (l->expr_is == EXPR_RVAL && (l->op == OP_PLUS || l->op == OP_MINUS)) {
                    if (l->l1->expr_is == EXPR_LVAL && l->l2->expr_is != EXPR_HARDCODED_CONST) return 0;
                    if (l->l2->expr_is == EXPR_LVAL && l->l1->expr_is != EXPR_HARDCODED_CONST) return 0;
                } else if (l->expr_is != EXPR_LVAL) {
                    return 0;
                }
            }
        }

        current = current->next;
    }

    return 1;
}

void dependence_analysis(statement_t *block) {
    switch (block->type) {
    case ST_If:
        block->_if._true->dep = do_dependence_analysis(block->_if._true);
        if (block->_if._false)
            block->_if._false->dep = do_dependence_analysis(block->_if._false);
        break;
    case ST_While:
        //there is no prologue or epilogue yet
        block->dep = do_dependence_analysis(block->_while.loop);
        break;
    case ST_For:
        block->dep->guard = block->_for.var;

        block->_for.unroll_me = can_unroll_this_for_stmt(block);

        //there is no prologue or epilogue yet
        block->dep = do_dependence_analysis(block->_for.loop);
        break;
    case ST_Comp:
        block->dep = do_dependence_analysis(block);
        break;
    default:
        die("UNEXPECTED_ERROR: do_dependence_analysis(): expected comp statement");
    }
}
