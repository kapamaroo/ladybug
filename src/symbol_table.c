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

//ARRAY implementation of symbol table
sem_t **sm_array;
sem_t **sm_table;
int sm_empty = MAX_SYMBOLS;

sem_t *sem_INTEGER;
sem_t *sem_REAL;
sem_t *sem_BOOLEAN;
sem_t *sem_CHAR;

var_t *lost_var;

func_t *create_main_program(char *name) {
    sem_t *sem_main_program;

    sem_main_program = sm_insert(name);
    sem_main_program->id_is = ID_PROGRAM_NAME;

    //see scope.h
    main_program = (func_t*)malloc(sizeof(func_t));
    main_program->status = FUNC_USEFULL;

    //main_program->func_name = sem_main_program->name;
    main_program->func_name = "main";
    sem_main_program->subprogram = main_program;

    new_statement_module(main_program);

    return sem_main_program->subprogram;
}

void init_symbol_table() {
    data_t *void_datatype; //datatype of lost symbols

#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
    printf("Initializing symbol table.. ");
#endif

    sm_array = (sem_t**)malloc(MAX_SYMBOLS*sizeof(sem_t*));
    sm_table = sm_array;

    init_scope();

    //at the begining there is no memory
    idf_table = (idf_t*)malloc(MAX_IDF*sizeof(idf_t));
    idf_init(IDF_KEEP_MEM);

    usr_datatype = (data_t*)malloc(sizeof(data_t));

    void_datatype = (data_t*)malloc(sizeof(struct data_t));
    void_datatype->is = TYPE_VOID;
    void_datatype->def_datatype = void_datatype;
    void_datatype->data_name = "__void_datatype__";
    void_datatype->memsize = 0;

    //insert the standard types
    sem_INTEGER = sm_insert("integer");
    sem_REAL = sm_insert("real");
    sem_BOOLEAN = sm_insert("boolean");
    sem_CHAR = sm_insert("char");

    sem_INTEGER->id_is = ID_TYPEDEF;
    sem_REAL->id_is = ID_TYPEDEF;
    sem_BOOLEAN->id_is = ID_TYPEDEF;
    sem_CHAR->id_is = ID_TYPEDEF;

    sem_INTEGER->comp = (data_t*)malloc(sizeof(data_t));
    sem_REAL->comp = (data_t*)malloc(sizeof(data_t));
    sem_BOOLEAN->comp = (data_t*)malloc(sizeof(data_t));
    sem_CHAR->comp = (data_t*)malloc(sizeof(data_t));

    sem_INTEGER->comp->is = TYPE_INT;
    sem_REAL->comp->is = TYPE_REAL;
    sem_BOOLEAN->comp->is = TYPE_BOOLEAN;
    sem_CHAR->comp->is = TYPE_CHAR;

    //point to itself
    sem_INTEGER->comp->def_datatype = sem_INTEGER->comp;
    sem_REAL->comp->def_datatype = sem_REAL->comp;
    sem_BOOLEAN->comp->def_datatype = sem_BOOLEAN->comp;
    sem_CHAR->comp->def_datatype = sem_CHAR->comp;

    sem_INTEGER->comp->data_name = sem_INTEGER->name;
    sem_REAL->comp->data_name = sem_REAL->name;
    sem_BOOLEAN->comp->data_name = sem_BOOLEAN->name;
    sem_CHAR->comp->data_name = sem_CHAR->name;

    sem_INTEGER->comp->memsize = MEM_SIZEOF_INT;
    sem_REAL->comp->memsize = MEM_SIZEOF_REAL;
    sem_BOOLEAN->comp->memsize = MEM_SIZEOF_BOOLEAN;
    sem_CHAR->comp->memsize = MEM_SIZEOF_CHAR;

    //(d->is==TYPE_ARRAY && d->field_num==1 && d->def_datatype->is==TYPE_CHAR)
    VIRTUAL_STRING_DATATYPE = (data_t*)malloc(sizeof(data_t));
    VIRTUAL_STRING_DATATYPE->is = TYPE_ARRAY;
    VIRTUAL_STRING_DATATYPE->field_num = 1;
    VIRTUAL_STRING_DATATYPE->def_datatype = SEM_CHAR;

    lost_var = (var_t*)malloc(sizeof(var_t));
    lost_var->id_is = ID_LOST;
    lost_var->name = "__lost_variable__";
    lost_var->datatype = void_datatype;
    //lost_var->scope = &scope_stack[0]; //lost symbols are adopted by main_program_scope
    lost_var->ival = 0;
    lost_var->fval = 0;
    lost_var->cval = 0;
    lost_var->Lvalue = (mem_t*)malloc(sizeof(mem_t));
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

    //search backwards in order to search in current scope first
    for (i=MAX_SYMBOLS-sm_empty-1;i>=0;i--) {
        if (sm_table[i] != NULL) {
            //search in symbols' names
            if (strcmp(id,sm_table[i]->name)==0) {
                return sm_table[i];
            }
            //search in composite types for id
            if (sm_table[i]->id_is==ID_TYPEDEF) {
                if (sm_table[i]->comp->is==TYPE_ENUM
                    /*|| sm_table->comp->def_type==D_RECORD*/) {
                    for (j=0;j<sm_table[i]->comp->field_num;j++) {
                        if (strcmp(id,sm_table[i]->comp->field_name[j])==0) {
                            return sm_table[i];
                        }
                    }
                }
            }
        }
        else {
            //no more symbols, return
            sm_insert_lost_symbol(id);
            return NULL;
        }
    }
    return NULL;
}

sem_t *sm_insert(const char *id) {
    //sets only the name of the symbol
    sem_t *existing_sem;
    sem_t *new_sem;

    existing_sem = sm_find(id);
    if ((!existing_sem || existing_sem->scope!=get_current_scope() || root_scope_with) && sm_empty) {
        new_sem = (sem_t*)malloc(sizeof(sem_t));
        new_sem->name = strdup(id);
        new_sem->scope = get_current_scope();
        new_sem->index = MAX_SYMBOLS-sm_empty;
        sm_table[MAX_SYMBOLS-sm_empty] = new_sem;
        sm_empty--;
        //printf("__symbol__ `%s` inserted :empty=%d\n",id,sm_empty);
        return new_sem;
    }
    else if (!sm_empty) {
        yyerror("symbol table is full, cannot insert new symbols.");
        exit(EXIT_FAILURE);
    }
    else {
        sprintf(str_err,"`%s` already declared in this scope",id);
        yyerror(str_err);
        return NULL;
    }
}

void sm_remove(char *id) {
    //int i;
    sem_t *symbol;
    func_t *scope_owner;

    symbol = sm_find(id);

    if (!symbol) {
        die("INTERNAL ERROR: trying to remove null symbol from symbol table");
    }

    //INFO: do not free() the elements, just remove them from symbol_table

    switch (symbol->id_is) {
    case ID_VAR_GUARDED:
    case ID_RETURN:
    case ID_VAR:
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
        scope_owner = get_current_scope_owner();
        sprintf(str_err,"subprogram '%s' without body in module %s.",id,scope_owner->func_name);
        yyerror(str_err);
        break;
    }
    //free(symbol->name);
    free(symbol);
    symbol = NULL;
    sm_empty++;
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
        lost_id = sm_find_lost_symbol(id);
        if (!lost_id) {
            sm_insert_lost_symbol(id);
            //sprintf(str_err,"undeclared symbol '%s'",id);
            //yyerror(str_err);
        }
        sprintf(str_err,"non constant value in constant declaration of '%s'",id);
        yyerror(str_err);
        return;
    }

    if (!TYPE_IS_STANDARD(l->datatype) && l->datatype->is!=TYPE_ENUM) {
        //we have only one value so it cannot be a subset
        yyerror("invalid nonstandard datatype of constant declaration");
        lost_id = sm_find_lost_symbol(id);
        if (!lost_id) {
            sm_insert_lost_symbol(id);
            //sprintf(str_err,"undeclared symbol '%s'",id);
            //yyerror(str_err);
        }
        return;
    }

    sem = sm_insert(id);
    if (sem) {
        sem->id_is = ID_CONST;
        new_var = (var_t*)malloc(sizeof(var_t));
        new_var->id_is = ID_CONST;
        new_var->datatype = l->datatype;
        new_var->name = sem->name;
        new_var->ival = l->ival;
        new_var->fval = l->fval;
        new_var->cval = l->cval;
        //new_var->cstr = l->cstr;
        new_var->scope = sem->scope;
        sem->var = new_var;

        sem->var->Lvalue = mem_allocate_symbol(l->datatype);
    }
    else {
        sprintf(str_err,"'%s' already declared",id);
        yyerror(str_err);
    }
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

    if (type) {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            new_sem = sm_insert(idf_table[i].name);
            if (new_sem) {
                new_sem->id_is = ID_VAR;
                new_sem->var = (var_t*)malloc(sizeof(var_t));
                new_sem->var->id_is = ID_VAR;
                new_sem->var->datatype = type;
                new_sem->var->name = new_sem->name;
                new_sem->var->Lvalue = mem_allocate_symbol(type);

                new_sem->var->status_value = VALUE_GARBAGE;
                new_sem->var->status_use = USE_NONE;
                new_sem->var->status_known = KNOWN_NO;
            }
            else {
                sprintf(str_err,"'%s' already declared",idf_table[i].name);
                yyerror(str_err);
            }
        }
    }
    else {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            lost_id = sm_find_lost_symbol(idf_table[i].name);
            if (!lost_id) {
                sm_insert_lost_symbol(idf_table[i].name);
                //sprintf(str_err,"undeclared symbol '%s'",id);
                //yyerror(str_err);
            }
        }
#if BISON_DEBUG_LEVEL >= 1
        yyerror("trying to declare variable(s) of unknown datatype (debugging info)");
#endif
    }

    idf_init(IDF_FREE_MEM);
}

void declare_formal_parameters(func_t *subprogram) {
    int i;

    sem_t *new_sem;
    var_t *new_var;

    //we do not put the variables in the stack here, just declare them in scope and allocate them
    for (i=0;i<subprogram->param_num;i++) {
        new_sem = sm_insert(subprogram->param[i]->name);
        new_sem->id_is = ID_VAR;

        new_var = (var_t*)malloc(sizeof(var_t));
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

void sm_insert_lost_symbol(const char *id) {
    char **pool;
    char *tmp;
    scope_t *current_scope;

    tmp = sm_find_lost_symbol(id);
    if (tmp) {
        return;
    }

    current_scope = get_current_scope();
    pool = current_scope->lost_symbols;

    if (current_scope->lost_symbols_empty>0) {
        tmp = strdup(id);
        pool[MAX_LOST_SYMBOLS - current_scope->lost_symbols_empty] = tmp;
        current_scope->lost_symbols_empty--;
    }
    else {
        die("FATAL_ERROR: reached maximun lost symbols from current scope");
    }
}

char *sm_find_lost_symbol(char *id) {
    char **pool;
    int index;
    int i;
    scope_t *current_scope;

    current_scope = get_current_scope();
    pool = current_scope->lost_symbols;

    index = MAX_LOST_SYMBOLS - current_scope->lost_symbols_empty;

    if (index>0) {
        for (i=index-1;i>=0;i--) {
            if (pool[i]==NULL) {
                return NULL;
            }
            else if (strcmp(pool[i],id)==0) {
                return pool[i];
            }
        }
    }
    return NULL; //no lost symbols
}

var_t *protect_guard_var(char *id) {
    sem_t *sem_2;

    sem_2 = sm_find(id);
    if (sem_2) {
        if (sem_2->id_is==ID_VAR) {
            if (sem_2->var->datatype->is==TYPE_INT) {
                sem_2->var->id_is = ID_VAR_GUARDED;
                return sem_2->var;
            }
            else {
                sprintf(str_err,"control variable '%s' must be integer",id);
                yyerror(str_err);
            }
        }
        else if (sem_2->id_is==ID_VAR_GUARDED) {
            sprintf(str_err,"variable '%s' already controls a for statement",id);
            yyerror(str_err);
        }
        else {
            sprintf(str_err,"invalid reference to '%s', expected variable",id);
            yyerror(str_err);
        }
    }
    //else
    //nothing, error is printed from sm_insert
    //yyerror("the name of the control variable is declared before in this scope");
    return NULL;
}

void unprotect_guard_var(var_t *var) {
    if (var) {
        var->id_is = ID_VAR;
    }
}

var_t *lost_var_reference() {
    return lost_var;
}
