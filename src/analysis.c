#include <stdio.h>
#include <stdlib.h>

#include "build_flags.h"
#include "analysis.h"
#include "statements.h"
#include "expr_toolbox.h"

void do_analyse_block(statement_t *block);
var_list_t *do_expr_list_analysis(var_list_t *var_list, expr_list_t *expr_list);

void dependence_analysis(statement_t *block);
dep_vector_t *do_dependence_analysis(dep_vector_t *dep, statement_t *comp_stmt);

#if (BISON_DEBUG_LEVEL >=1)
void debug_print_block_analysis(statement_t *block);
void debug_print_dependence_vectors(statement_t *block);
#endif

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

        while (pending && NEW_STMT_BLOCK_STARTS_FROM(pending))
            pending = pending->next;

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
    block->io_vectors.read =
        merge_var_lists(
                        block->io_vectors.read,
                        stmt->io_vectors.read);

    block->io_vectors.write =
        merge_var_lists(
                        block->io_vectors.write,
                        stmt->io_vectors.write);
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
            /*
            if (l->var->from_comp && l->var->from_comp->comp_type==TYPE_ARRAY)
                var_list = insert_to_var_list(var_list,l->var->from_comp->array.base);
            else
            */
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
    s->io_vectors.write =
        var_list_add(s->io_vectors.write,s->_assignment.var);

    s->io_vectors.read =
        do_expr_tree_analysis(s->io_vectors.read,s->_assignment.expr);
}

void do_call_analysis(statement_t *s) {
    //reminder: this is a procedure call, no write dependencies

    s->io_vectors.read =
        do_expr_list_analysis(s->io_vectors.read,s->_call.expr_params);

}

void do_read_analysis(statement_t *s) {
    //read stmt has only write dependencies

    //variable list is ready, see bison.y
    s->io_vectors.write = s->_read.var_list;
}

void do_write_analysis(statement_t *s) {
    //write stmt has only read dependencies

    s->io_vectors.read =
        do_expr_list_analysis(s->io_vectors.read,s->_write.expr_list);

}

void do_analyse_stmt(statement_t *s,statement_t *block) {
    /*
#if (BISON_DEBUG_LEVEL >=1)
    int i=0;
    int j=0;
#endif
    */

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

        //set statement id
        s->stat_id = block->size++;

        /*
#if (BISON_DEBUG_LEVEL >=1)
        printf("debug:\t\t\tstmt %d:",i);
        if (s->io_vectors.write) {
            printf("\t\tOUT_VECTOR(");
            for (j=0;j<s->io_vectors.write->all_var_num;j++) {
                printf("%s,",s->io_vectors.write->var_list[j]->name);
            }
            printf(")");
        } else {
            //align output
            printf("\t\t\t");
        }

        if (s->io_vectors.read) {
            printf("\t\tIN_VECTOR(");
            for (j=0;j<s->io_vectors.read->all_var_num;j++) {
                printf("%s,",s->io_vectors.read->var_list[j]->name);
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
        block->io_vectors.read =
            do_expr_tree_analysis(
                                  block->io_vectors.read,
                                  block->_if.condition);

        do_analyse_stmt(block->_if._true,block->_if._true);
        do_analyse_stmt(block->_if._false,block->_if._false);

        merge_stmt_analysis_to_block(block->_if._true,block);
        merge_stmt_analysis_to_block(block->_if._false,block);
        break;
    case ST_While:
        block->io_vectors.read =
            do_expr_tree_analysis(
                                  block->io_vectors.read,
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
        die("UNEXPECTED_ERROR: expected block for analysis");
    }

    block->depth = nesting;

    nesting--;
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
#if (BISON_DEBUG_LEVEL >=1)
            debug_print_block_analysis(current);
#endif
            dependence_analysis(current);

#if (BISON_DEBUG_LEVEL >=1)
            debug_print_dependence_vectors(current);
#endif

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
        }

        if (l->var==guard) {
            return 1;
        }
        break;
    default:
        break;
    }

    return 0;
}

inline int VAR_GUARD_CONTROLS_ARRAY_ELEMENT_INDEX(var_t *guard, expr_list_t *index) {
    int i;

    if (!guard)
        return 0;

    for (i=0; i<index->all_expr_num; i++)
        if (EXPR_CONTAINS_GUARD(guard,index->expr_list[i]))
            return i+1;

    return 0;
}

inline info_comp_t *VAR_IS_GUARDED_ARRAY_ELEMENT(var_t *guard, var_t *v) {
    info_comp_t *var_of;

    var_of = v->from_comp;

    while (var_of) {
        if (var_of->comp_type==TYPE_ARRAY) {
            int conflict_pos = VAR_GUARD_CONTROLS_ARRAY_ELEMENT_INDEX(guard,var_of->array.index);
            if (conflict_pos) {
                var_of->array.index_conflict_pos = conflict_pos;
                return var_of;
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

inline int GUARDED_VARS_OF_THE_SAME_VAR_ARRAY(dep_vector_t *dep, var_t *var_from, var_t *var_to) {
    info_comp_t *var_of1;
    info_comp_t *var_of2;

    if (var_from == var_to)
        return 1;

    var_of1 = VAR_IS_GUARDED_ARRAY_ELEMENT(dep->guard,var_from);
    var_of2 = VAR_IS_GUARDED_ARRAY_ELEMENT(dep->guard,var_to);

    if (var_of1 && var_of2 &&
        var_of1->array.base == var_of2->array.base) {
        dep->pool[dep->next_free_spot].conflict_info_from = var_of1;
        dep->pool[dep->next_free_spot].conflict_info_to = var_of2;
        return 1;
    }

    return 0;
}

inline int VARS_MAY_CONFLICT(var_t *var_from, var_t *var_to) {
    if (var_from == var_to)
        return 1;

    if (var_from->from_comp && var_to->from_comp)
        if (var_from->from_comp->comp_type==TYPE_ARRAY &&
            var_to->from_comp->comp_type==TYPE_ARRAY)
            if (var_from->from_comp->array.base==
                var_to->from_comp->array.base)
                return 1;

    return 0;
}

inline int VAR_IS_WRITTEN_MEANWHILE(statement_t *from, statement_t *to, var_t *var_from) {
    int i;
    statement_t *tmp;
    var_list_t *var_list;

    tmp = from;

    while (tmp != to) {
        var_list = tmp->io_vectors.write;

        //some statements do not have side effects (e.g. procedure calls)
        if (var_list)
            for (i=0; i<var_list->all_var_num; i++) {
                var_t *v = var_list->var_list[i];
                if (var_from == v)
                    return 1;
            }

        tmp = tmp->next;
    }

    return 0;
}

void find_dependencies(dep_vector_t *dep, statement_t *from, statement_t *to, enum dependence_type dep_type) {
    int i;
    int j;

    var_list_t *from_list;
    var_list_t *to_list;

    switch (dep_type) {
    case DEP_RAR:
        to_list = to->io_vectors.read;
        from_list = from->io_vectors.read;
        break;
    case DEP_RAW:
        to_list = to->io_vectors.read;
        from_list = from->io_vectors.write;
        break;
    case DEP_WAR:
        to_list = to->io_vectors.write;
        from_list = from->io_vectors.read;
        break;
    case DEP_WAW:
        to_list = to->io_vectors.write;
        from_list = from->io_vectors.write;
        break;
    }

    //no dependencies
    if (!from_list || !to_list) {
        return;
    }

    for (i=0; i<from_list->all_var_num; i++) {

        var_t *var_from = from_list->var_list[i];

        if (dep_type == DEP_RAR || dep_type == DEP_WAR)
            if (VAR_IS_WRITTEN_MEANWHILE(from,to,var_from))
                continue;

        for (j=0; j<to_list->all_var_num; j++) {
            var_t *var_to = to_list->var_list[j];

            if (VARS_MAY_CONFLICT(var_from,var_to)) {
                //printf("debug:\t__new_dep__\n");
                //commit dependence
                dep_t *this_dep = &dep->pool[dep->next_free_spot];
                this_dep->is = dep_type;
                this_dep->from = from;
                this_dep->to = to;
                this_dep->index = dep->next_free_spot;
                this_dep->var_from = var_from;
                this_dep->var_to = var_to;

                if (dep->guard) {
                    //extra info for loop analysis
                    if (GUARDED_VARS_OF_THE_SAME_VAR_ARRAY(dep,var_from,var_to)) {

                        //maybe we must swap the dependence according to index relation
                        expr_t *l;

                        l = this_dep->conflict_info_from->array.index->expr_list[0];
                        int from_iter = 0;
                        if (l->expr_is==EXPR_RVAL)
                            from_iter = l->l2->ival;

                        int to_iter = 0;
                        l = this_dep->conflict_info_to->array.index->expr_list[0];
                        if (l->expr_is==EXPR_RVAL)
                            to_iter = l->l2->ival;

                        //invert dependence
                        if (from_iter > to_iter) {
                            if (dep_type==DEP_RAW)
                                this_dep->is = DEP_WAR;
                            else if (dep_type==DEP_WAR)
                                this_dep->is = DEP_RAW;

                            this_dep->from = to;
                            this_dep->to = from;
                        }
                    }
                }

                //commit new dependence
                dep->next_free_spot++;
            }
        }
    }
}

int estimate_num_of_dependencies(statement_t *current) {
    //upper bound

    int counter = 0;

    while (current) {
        if (current->io_vectors.write)
            counter += current->io_vectors.write->all_var_num;

        if (current->io_vectors.read)
            counter += current->io_vectors.read->all_var_num;

        current = current->next;
    }

    return counter;
}

dep_vector_t *do_dependence_analysis(dep_vector_t *dep, statement_t *stmt) {
    int estimate;
    statement_t *from;
    statement_t *to;

    if (stmt->type==ST_Comp)
        from = stmt->_comp.first_stmt;
    else
        from = stmt;

    estimate = estimate_num_of_dependencies(from);

    dep->pool = (dep_t*)calloc(estimate,sizeof(dep_t));

    while (from) {

        to = from;
        //self statement dependencies between lvalue and rvalue
        find_dependencies(dep,from,to,DEP_WAR);
        to = to->next;

        while (to) {

            find_dependencies(dep,from,to,DEP_RAW);
            find_dependencies(dep,from,to,DEP_WAR);

            find_dependencies(dep,from,to,DEP_RAR);
            find_dependencies(dep,from,to,DEP_WAW);

            to = to->next;
        }

        from = from->next;
    }

    return dep;
}

inline int EXPR_IS_SIMPLE_ENOUGH(expr_t *l) {
    //reminder: we convert OP_MINUS to OP_PLUS when l->l2 is HARDCODED_CONST
    if (l->expr_is == EXPR_RVAL && l->op == OP_PLUS) {
        if (l->l1->expr_is != EXPR_LVAL ||
            l->l2->expr_is != EXPR_HARDCODED_CONST)
            return 0;
    } else if (l->expr_is != EXPR_LVAL) {
        return 0;
    }
}

int can_unroll_this_for_stmt(statement_t *block) {
    int i;
    statement_t *current;

    //support only special cases of arrays of 1-dimension

    current = block->_for.loop->_comp.first_stmt;

    while (current) {
        //accept only primitive assignment statements
        if (current->type != ST_Assignment)
            return 0;

        //for statement must not contain function calls
        for (i=0; i<current->io_vectors.read->all_var_num; i++) {
            var_t *v = current->io_vectors.read->var_list[i];
            if (v->id_is==ID_RETURN) {
                return 0;
            }

            if (v->from_comp && v->from_comp->comp_type==TYPE_ARRAY) {
                //no multiple dimension arrays
                //if (v->from_comp->array->index->all_expr_num > 1) {
                if (v->from_comp->array.base->datatype->field_num > 1) {
                    return 0;
                }

                //no nested arrays in datatypes
                if (v->from_comp->array.base->from_comp) {
                    return 0;
                }

                //simple enough index expression
                expr_t *l = v->from_comp->array.index->expr_list[0];
                if (!EXPR_IS_SIMPLE_ENOUGH(l))
                    return 0;
            }
        }

        for (i=0; i<current->io_vectors.write->all_var_num; i++) {
            var_t *v = current->io_vectors.write->var_list[i];
            if (v->from_comp && v->from_comp->comp_type==TYPE_ARRAY) {
                //no multiple dimension arrays
                if (v->from_comp->array.index->all_expr_num > 1) {
                    return 0;
                }

                //no nested arrays in datatypes
                if (v->from_comp->array.base->from_comp) {
                    return 0;
                }

                //simple enough index expression
                expr_t *l = v->from_comp->array.index->expr_list[0];

                if (!EXPR_IS_SIMPLE_ENOUGH(l))
                    return 0;
            }
        }

        current = current->next;
    }

    return 1;
}

void dependence_analysis(statement_t *block) {
    dep_vector_t *dep;

    dep = (dep_vector_t*)calloc(1,sizeof(dep_vector_t));

    switch (block->type) {
    case ST_If:
        block->_if._true->dep =
            do_dependence_analysis(dep,block->_if._true);

        if (block->_if._false)
            block->_if._false->dep =
                do_dependence_analysis(dep,block->_if._false);
        break;
    case ST_While:
        //there is no prologue or epilogue yet
        block->dep =
            do_dependence_analysis(dep,block->_while.loop);
        break;
    case ST_For:
        dep->guard = block->_for.var;

        block->_for.unroll_me = can_unroll_this_for_stmt(block);

        //there is no prologue or epilogue yet
        block->dep = do_dependence_analysis(dep,block->_for.loop);
        break;
    case ST_Comp:
        block->dep = do_dependence_analysis(dep,block);
        break;
    default:
        die("UNEXPECTED_ERROR: do_dependence_analysis(): expected comp statement");
    }
}

#if (BISON_DEBUG_LEVEL >=1)
void debug_print_block_analysis(statement_t *block) {
    printf("debug:\tblock-nesting: %d <==> statements %d\t",block->depth,block->size);

    int j;
    //printf("debug:\t\t\tblock:");
    if (block->io_vectors.write) {
        printf("\t\tOUT_VECTOR(");
        for (j=0;j<block->io_vectors.write->all_var_num;j++) {
            printf("%s,",block->io_vectors.write->var_list[j]->name);
        }
        printf(")");
    } else {
        //align output
        printf("\t\t\t");
    }

    if (block->io_vectors.read) {
        printf("\t\tIN_VECTOR(");
        for (j=0;j<block->io_vectors.read->all_var_num;j++) {
            printf("%s,",block->io_vectors.read->var_list[j]->name);
        }
        printf(")");
    }
    printf("\n");
}

void debug_print_dependence_vectors(statement_t *block) {
    int i;

    if (!block->dep) {
        printf("debug: __no_dep__\n");
        return;
    }

    for (i=0; i<block->dep->next_free_spot; i++) {
        dep_t *this_dep = &block->dep->pool[i];
        printf("debug: ");
        switch (this_dep->is) {
        case DEP_RAR:  printf("DEP_RAR\t");  break;
        case DEP_RAW:  printf("DEP_RAW\t");  break;
        case DEP_WAR:  printf("DEP_WAR\t");  break;
        case DEP_WAW:  printf("DEP_WAW\t");  break;
        }

        //print statements
        printf("S%d <--> S%d",
               this_dep->from->stat_id,
               this_dep->to->stat_id);

        char *name_from;
        char *name_to;

        if (this_dep->conflict_info_from)
            name_from = this_dep->conflict_info_from->array.base->name;
        else
            name_from = this_dep->var_from->name;

        if (this_dep->conflict_info_to)
            name_to = this_dep->conflict_info_to->array.base->name;
        else
            name_to = this_dep->var_to->name;

        //print conflicting variable
        printf(" (%s,%s)",name_from,name_to);

        printf("\n");
    }
}

#endif
