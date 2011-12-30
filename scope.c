#include <stdio.h>
#include <stdlib.h>

#include "semantics.h"
#include "bison.tab.h"
#include "scope.h"
#include "symbol_table.h"
#include "err_buff.h"

int sm_scope;
scope_t scope_stack[MAX_SCOPE+1];

with_stmt_scope_t *root_scope_with;
with_stmt_scope_t *tail_scope_with;

void init_scope() {
    int i;

    scope_stack[0].scope_owner = sem_main_program->subprogram;
    scope_stack[0].start_index = 0;
    scope_stack[0].lost_symbols = (char**)malloc(MAX_LOST_SYMBOLS*sizeof(char*));
    scope_stack[0].lost_symbols_empty = MAX_LOST_SYMBOLS;

    for (i=0;i<MAX_LOST_SYMBOLS;i++) {
        scope_stack[0].lost_symbols[i] = NULL;
    }

    root_scope_with = NULL;
    tail_scope_with = NULL;

    sm_scope = 0; //main scope

    for (i=1;i<MAX_SCOPE+1;i++) {
        scope_stack[i].scope_owner = NULL;
        scope_stack[i].lost_symbols = NULL;
    }
}

void start_new_scope(func_t *scope_owner) {
    int i;

    //the main_scope is created in init_symbol_table()
    //create scopes nested to main program's scope

    //create new scope even if symbol table is full, let the next sm_insert() handle this case

    if (sm_scope<MAX_SCOPE) {
        sm_scope++;
        scope_stack[sm_scope].scope_owner = scope_owner;
        scope_stack[sm_scope].lost_symbols_empty = MAX_LOST_SYMBOLS;
        scope_stack[sm_scope].lost_symbols = (char**)malloc(MAX_LOST_SYMBOLS*sizeof(char*));

        for (i=0;i<MAX_LOST_SYMBOLS;i++) {
            scope_stack[sm_scope].lost_symbols[i] = NULL;
        }

        scope_stack[sm_scope].start_index = sm_table[MAX_SYMBOLS-sm_empty-1]->index + 1;
#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
        printf("__start_new_scope_%d\n",sm_scope);
#endif
        return;
    }
    else {
        yyerror("FATAL_ERROR: reached max scope depth");
        exit(EXIT_FAILURE);
    }
}

void close_current_scope() {
    //every time a scope is closed we must clean the symbol table from its declarations and definitions
    //make sure all the declared subprobrams in this scope,
    //have one body in this scope too, (in the case of FORWARD)

    if (sm_scope<0) {
        //there is no main scope any more
        sprintf(str_err,"INTERNAL_ERROR: no scope to delete.");
        yyerror(str_err);
        exit(EXIT_FAILURE);
    }
    sm_clean_current_scope();

    sm_scope--;
#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
    printf("__close_current_scope_%d\n",sm_scope);
#endif
}

scope_t *get_current_scope() {
    return &scope_stack[sm_scope];
}

func_t *get_current_scope_owner() {
    return (func_t*)(scope_stack[sm_scope].scope_owner);
}

void sm_clean_current_scope() {
    int i;
    int scope_start;

    scope_start = scope_stack[sm_scope].start_index;
    i = MAX_SYMBOLS-sm_empty-1;
    for (;i>=scope_start;i--) {
        sm_remove(sm_table[i]->name);
    }

    //clean lost symbols of scope
    for (i=0;i<MAX_LOST_SYMBOLS;i++) {
        if (scope_stack[sm_scope].lost_symbols[i]!=NULL) {
            free(scope_stack[sm_scope].lost_symbols[i]);
        }
        else {
            //no more lost symbols
            break;
        }
    }
    free(scope_stack[sm_scope].lost_symbols);
    scope_stack[sm_scope].lost_symbols = NULL;
}

void start_new_with_statement_scope(var_t *var) {
    int i;
    sem_t *new_sem;
    with_stmt_scope_t *new_with_stmt_scope;
    with_stmt_scope_t *with_scope;

    //composite types are declared after constants, so if this
    //var_t pointer has a composite datatype, it is a variable for
    //sure. see semantics.h for more information about the
    //representation of variables and structs.

    if (!var) {
#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
        sprintf(str_err,"null variable in with_statement (debugging info)");
        yyerror(str_err);
        return;
#endif
    }

    if (var->datatype->is!=TYPE_RECORD) {
        yyerror("the type of variable is not a record");
        return;
    }

    if (!root_scope_with) { //this is the first with_statement
        root_scope_with = (with_stmt_scope_t*)malloc(sizeof(with_stmt_scope_t));
        root_scope_with->prev = root_scope_with;
        root_scope_with->next = NULL;
        tail_scope_with = root_scope_with;
    }
    else { //this is a nested with_statement
        //check for possible conflicts between nested with_statements
        for(with_scope=root_scope_with;with_scope;with_scope=with_scope->next) {
            for(i=0;i<var->datatype->field_num;i++) {
                if (check_for_id_in_datatype(with_scope->type,var->datatype->field_name[i])>=0) {
                    sprintf(str_err,"conflict in nested with_statement of type '%s' inside type '%s',\n\t both record datatypes have the name '%s' for element"
                            ,var->datatype->data_name,with_scope->type->data_name,var->datatype->field_name[i]);
                    yyerror(str_err);
                    tail_scope_with->conflicts++;;
                    return; //do not insert any record element, do not open new with_scope
                }
            }
        }
        //no conficts, add a new with_scope
        new_with_stmt_scope = (with_stmt_scope_t*)malloc(sizeof(with_stmt_scope_t));
        new_with_stmt_scope->prev = tail_scope_with;
        new_with_stmt_scope->next = NULL;
        tail_scope_with->next = new_with_stmt_scope;
        tail_scope_with = new_with_stmt_scope;
    }

    //a new with_scope just created!
    //common actions for both cases
    //set the datatype of the with statement
    tail_scope_with->type = var->datatype;
    tail_scope_with->conflicts = 0;

    //insert the record elements
    for (i=0;i<var->datatype->field_num;i++) {
        new_sem = sm_insert(var->datatype->field_name[i]);
        new_sem->var = refference_to_record_element(var,var->datatype->field_name[i]);
    }
}

void close_last_opened_with_statement_scope() {
    int i;

    //with_scopes close backwards (the last with scope closes first)
    //so we play with tail_scope_with

    //reminder: recursive data types are forbidden (do NOT exist by language definition)

    if (!root_scope_with || !tail_scope_with) {
        //root_scope_with and tail_scope_with are not initialised,
        //someone called this function without calling the start_new_with_statement_scope() first
        yyerror("INTERNAL_ERROR: trying to close null with statement scope (debugging info)");
        exit(EXIT_FAILURE);
    }

    if (tail_scope_with->conflicts>0) {
        //we are trying to close a with scope which failed to open
        //and this is the point it should normally close
        //so the previous with_scope has one less conflict to carry about
        tail_scope_with->conflicts--;
    }
    else {
        //remove backwards to find the symbol entry faster
        //and also the corrent symbol if there is a predefined symbol with the same name
        for (i=tail_scope_with->type->field_num-1;i>=0;i--) {
            sm_remove(tail_scope_with->type->field_name[i]);
        }

        if (tail_scope_with==root_scope_with) { //the outermost with scope
            free(tail_scope_with);
            root_scope_with = NULL;
            tail_scope_with = NULL;
        }
        else {
            tail_scope_with = tail_scope_with->prev;
            free(tail_scope_with->next);
            tail_scope_with->next = NULL;
        }
    }
}
