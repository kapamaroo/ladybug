#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "build_flags.h"
#include "semantics.h"
#include "bison.tab.h"
#include "symbol_table.h"
#include "identifiers.h"
#include "datatypes.h"
#include "scope.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "mem.h"
#include "err_buff.h"

var_t *lost_var;

func_t *create_main_program(char *name) {
    sem_t *sem_main_program;

    sem_main_program = sm_insert(name,ID_PROGRAM_NAME);

    //see scope.h
    main_program = (func_t*)calloc(1,sizeof(func_t));
    main_program->status = FUNC_USEFULL;

    //reminder: flex called strdup, but we do not use it
    //free(name);

    //main_program->name = sem_main_program->name;
    main_program->name = "main";
    sem_main_program->subprogram = main_program;

    new_statement_module(main_program);

    return sem_main_program->subprogram;
}

void init_symbol_table() {
#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
    printf("Initializing symbol table.. ");
#endif

    init_scope();
    idf_init(IDF_KEEP_MEM);

    init_datatypes();

    lost_var = (var_t*)calloc(1,sizeof(var_t));
    lost_var->id_is = ID_LOST;
    lost_var->name = "__lost_variable__";
    lost_var->datatype = void_datatype;
    //lost_var->scope = &scope_stack[0]; //lost symbols are adopted by main_program_scope
    lost_var->ival = 0;
    lost_var->fval = 0;
    lost_var->cval = 0;
    lost_var->Lvalue = (mem_t*)calloc(1,sizeof(mem_t));
    lost_var->Lvalue->content_type = PASS_VAL;
    lost_var->Lvalue->offset_expr = expr_from_hardcoded_int(0);
    lost_var->Lvalue->seg_offset = expr_from_hardcoded_int(0);
    lost_var->Lvalue->segment = MEM_STACK;
    lost_var->Lvalue->size = 0;

#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
    printf("OK\n");
#endif
}

sem_t *sm_find(const char *id) {
    //do not search the names of functions' parameters because they physically belong to other scope
    //do not search in record's elements
    //search in enumerations
    int i;
    int j;
    int size;

    func_t *scope_owner;
    sem_t **sm_table;

    scope_owner = get_current_scope_owner();
    while (scope_owner) {
        //printf("debug: search symbol '%s' in scope '%s'\n", id, scope_owner->name);

        size = MAX_SYMBOLS - scope_owner->symbol_table.pool_empty;
        sm_table = scope_owner->symbol_table.pool;

        //backwards search to find the correct symbol (in case of with_statement name overloading)
        for (i=size-1;i>=0;i--) {
            if (sm_table[i]) {
                //search in symbols' names
                if (strcmp(id,sm_table[i]->name)==0) {
                    return sm_table[i];
                }
                //search in composite types for id
                if (sm_table[i]->id_is==ID_TYPEDEF) {
                    if (sm_table[i]->comp->is==TYPE_ENUM
                        /*|| sm_table[i]->comp->def_type==D_RECORD*/) {
                        for (j=0;j<sm_table[i]->comp->field_num;j++) {
                            if (strcmp(id,sm_table[i]->comp->field_name[j])==0) {
                                return sm_table[i];
                            }
                        }
                    }
                }
            }
        }

        scope_owner = scope_owner->scope;
    }

    //sm_insert_lost_symbol(id,NULL);

    return NULL;
}

sem_t *sm_insert(char *id, idt_t ID_TYPE) {
    //sets only the name of the symbol
    sem_t *existing_sem;
    sem_t *new_sem;

    func_t *scope_owner;
    sem_t **sm_table;
    int sm_empty;

    scope_owner = get_current_scope_owner();
    sm_table = scope_owner->symbol_table.pool;
    sm_empty = scope_owner->symbol_table.pool_empty;

    existing_sem = sm_find(id);

    if ((!existing_sem || existing_sem->scope!=scope_owner || root_scope_with) && sm_empty) {
        new_sem = (sem_t*)calloc(1,sizeof(sem_t));
        new_sem->id_is = ID_TYPE;
        new_sem->name = id; //strdup(id);
        new_sem->scope = scope_owner;
        new_sem->index = MAX_SYMBOLS - sm_empty;
        sm_table[MAX_SYMBOLS - sm_empty] = new_sem;
        scope_owner->symbol_table.pool_empty--; //decrease the real counter
        //printf("debug: inserted symbol `%s` inserted in scope '%s'\n", id ,scope_owner->name);
        return new_sem;
    }
    else if (!sm_empty) {
        yyerror("symbol table is full, cannot insert new symbols.");
        exit(EXIT_FAILURE);
    }
    else {
        sprintf(str_err,"`%s` already declared in this scope",id);
        yyerror(str_err);
        return existing_sem; //for more inspection
    }
}

void sm_remove(char *id) {
    //int i;
    int index;
    sem_t *symbol;
    func_t *scope_owner;

    scope_owner = get_current_scope_owner();
    symbol = sm_find(id);

    if (!symbol) {
        die("INTERNAL ERROR: trying to remove null symbol from symbol table");
    }

    //INFO: do not free() the elements, just remove them from symbol_table

    switch (symbol->id_is) {
    case ID_VAR:
        if (symbol->var->status_use==USE_NONE) {
            sprintf(str_err, "variable '%s' of type '%s' not used in '%s'", symbol->var->name,
                    symbol->var->datatype->name,
                    symbol->var->scope->name);
            yywarning(str_err);
        }
    case ID_RETURN:
        //free(symbol->var);
        break;
    case ID_LOST:
    case ID_PROGRAM_NAME:
        break;
    case ID_STRING:
        //free(symbol->var->cstr);
        break;
    case ID_CONST:
        break;
    case ID_FUNC:
    case ID_PROC:
        //for (i=0;i<symbol->subprogram->param_num;i++) {
        //    free(symbol->subprogram->param[i]->name);
        //    free(symbol->subprogram->param[i]);
        //}
        //free(symbol->subprogram);
        break;
    case ID_TYPEDEF:
        //for (i=0;i<symbol->comp->field_num;i++) {
        //    free(symbol->comp->field_name[i]);
        //}
        break;
    case ID_FORWARDED_FUNC:
    case ID_FORWARDED_PROC:
        sprintf(str_err,"subprogram '%s' without body in module %s.",id,scope_owner->name);
        yyerror(str_err);
        break;
    case ID_VAR_GUARDED: //this is a virtual variable type, we should never see it here
    default:
        die("INTERNAL_ERROR: in sm_remove()");
    }
    //free(symbol->name);
    index = symbol->index;
    free(scope_owner->symbol_table.pool[index]);
    scope_owner->symbol_table.pool[index] = NULL;
    scope_owner->symbol_table.pool_empty++;
    //printf("debug: remove symbol '%s' of scope '%s'\n", symbol->name, scope_owner->name);
}

void declare_consts(char *id,expr_t *l) {
    sem_t *sem;
    var_t *new_var;
    char *lost_id;

    if (!l) {
        die("UNEXPECTED_ERROR: 11");
    }
    if (!l->datatype) {
        die("UNEXPECTED_ERROR: 12");
    }

    if (l->expr_is!=EXPR_HARDCODED_CONST) {
        sprintf(str_err,"non constant value in constant declaration of '%s'",id);
        sm_insert_lost_symbol(id,str_err);
        return;
    }

    if (!TYPE_IS_STANDARD(l->datatype) && l->datatype->is!=TYPE_ENUM) {
        //we have only one value so it cannot be a subset
        sprintf(str_err,"invalid non-standard datatype value in constant declaration of '%s'",id);
        sm_insert_lost_symbol(id,str_err);
        return;
    }

    sem = sm_insert(id,ID_CONST);
    if (!sem) {
        sprintf(str_err,"'%s' already declared",id);
        yyerror(str_err);
        free(id); //flex strdup'ed it
        return;
    }

    new_var = (var_t*)calloc(1,sizeof(var_t));
    new_var->id_is = ID_CONST;
    new_var->datatype = l->datatype;
    new_var->name = sem->name;
    new_var->ival = l->ival;
    new_var->fval = l->fval;
    new_var->cval = l->cval;
    //new_var->cstr = l->cstr;
    new_var->scope = sem->scope;

    new_var->status_value = VALUE_VALID;
    new_var->status_use = USE_NONE;
    new_var->status_known = KNOWN_YES;

    sem->var = new_var;

#warning we skip the allocation of constants
    //sem->var->Lvalue = mem_allocate_symbol(l->datatype);
}

void declare_vars(data_t* type){
    int i;
    sem_t *new_sem;
    char *lost_id;

    /* enable this if we want to declare vars when their type isn't declared right,
     * instead of inserting them in the island of lost symbols!
     */
    //      if (!type) {
    //              type = SEM_INTEGER;
    //      }

    if (!type) {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            sm_insert_lost_symbol(idf_table[i].name,NULL);
            //sprintf(str_err,"undeclared symbol '%s'",id);
            //yyerror(str_err);
        }

        return;
    }

    for (i=0;i<MAX_IDF-idf_empty;i++) {
        new_sem = sm_insert(idf_table[i].name,ID_VAR);
        if (new_sem) {
            //new_sem->var = (var_t*)calloc(1,sizeof(var_t));
            new_sem->var = (var_t*)calloc(1,sizeof(var_t));
            new_sem->var->id_is = ID_VAR;
            new_sem->var->datatype = type;
            new_sem->var->name = new_sem->name;
            new_sem->var->scope = new_sem->scope;
            new_sem->var->Lvalue = mem_allocate_symbol(type);
            new_sem->var->from_comp = NULL;

            new_sem->var->status_value = VALUE_GARBAGE;
            new_sem->var->status_use = USE_NONE;
            new_sem->var->status_known = KNOWN_NO;
        }
        else {
            sprintf(str_err,"'%s' already declared",idf_table[i].name);
            yyerror(str_err);
            free(idf_table[i].name); //flex strdup'ed it
        }
    }

    idf_init(IDF_KEEP_MEM);
}

void declare_formal_parameters(func_t *subprogram) {
    int i;

    sem_t *new_sem;
    var_t *new_var;

    //we do not put the variables in the stack here, just declare them in scope and allocate them
    for (i=0;i<subprogram->param_num;i++) {
        new_sem = sm_insert(subprogram->param[i]->name,ID_VAR);

        //should always succeed
        if (!new_sem) {
            die("INTERNAL_ERROR: declaration of formal parameter failed");
        }

        new_var = (var_t*)calloc(1,sizeof(var_t));
        new_var->id_is = ID_VAR;
        new_var->datatype = subprogram->param[i]->datatype;
        new_var->name = new_sem->name;
        new_var->scope = new_sem->scope;
        new_var->Lvalue = subprogram->param_Lvalue[i];

        new_var->status_value = VALUE_VALID;
        new_var->status_use = USE_YES;
        new_var->status_known = KNOWN_NO;

        new_sem->var = new_var;
    }
}

void sm_insert_lost_symbol(const char *id, const char *error_msg) {
    char **pool;
    char *tmp;
    func_t *current_scope;
    int empty;

    tmp = sm_find_lost_symbol(id);
    if (tmp) {
        return;
    }

    current_scope = get_current_scope_owner();
    pool = current_scope->symbol_table.lost;
    empty = current_scope->symbol_table.lost_empty;

    if (empty<=0) {
        die("FATAL_ERROR: reached maximun lost symbols for current scope");
    }

    //printf("debug: insert lost symbol '%s' in scope '%s'\n", id, current_scope->name);
    tmp = id; //strdup(id);
    pool[MAX_LOST_SYMBOLS - empty] = tmp;
    current_scope->symbol_table.lost_empty--;

    //print error if any
    if (error_msg) {
        yyerror(error_msg);
    }
}

char *sm_find_lost_symbol(const char *id) {
    char **pool;
    int index;
    int i;
    func_t *current_scope;

    current_scope = get_current_scope_owner();

    while (current_scope) {
        pool = current_scope->symbol_table.lost;
        index = MAX_LOST_SYMBOLS - current_scope->symbol_table.lost_empty;

        for (i=0;i<index;i++) {
            if (strcmp(pool[i],id)==0) {
                //printf("debug: found lost symbol '%s' in scope '%s'\n", id, current_scope->name);
                return pool[i];
            }
        }

        current_scope = current_scope->scope;
    }

    return NULL; //no lost symbols
}

var_t *lost_var_reference() {
    return lost_var;
}
