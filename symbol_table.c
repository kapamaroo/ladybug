#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "build_flags.h"
#include "semantics.h"
#include "bison.tab.h"
#include "symbol_table.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "mem_reg.h"
#include "err_buff.h"

//ARRAY implementation of symbol table
int sm_empty;
sem_t *sm_array[MAX_SYMBOLS];
sem_t **sm_table;
//sem_t *sm_table[MAX_SYMBOLS];
int sm_scope;
scope_t scope_stack[MAX_SCOPE+1];

sem_t *sem_main_program;
func_t *main_program;

sem_t *sem_INTEGER;
sem_t *sem_REAL;
sem_t *sem_BOOLEAN;
sem_t *sem_CHAR;

idf_t *idf_table[MAX_IDF];
data_t *idf_data_type;
int idf_empty = MAX_IDF;
int idf_free_memory;  //flag for idf_init() to free the memory of the symbols because sm_insert() copied called strdup

data_t *usr_datatype; //new user defined datatype
data_t *void_datatype; //return type of procedures
var_t *lost_var;

with_stmt_scope_t *root_scope_with;
with_stmt_scope_t *tail_scope_with;

idf_t *idf_find(const char *id) {
    int i;
    for (i=0;i<MAX_IDF;i++) {
        if (idf_table[i] != NULL) {
            if (strcmp(id,sm_table[i]->name)==0) {
                return idf_table[i];
            }
        }
        else {
            return NULL;
        }
    }
    return NULL;
}

int idf_insert(char *id) {
    //every identifier's id must be unique in the scope
    idf_t *new_idf;
    if (!idf_empty) {
        yyerror("ERROR: identifier table is full, cannot insert new id's.");
        return 0;
    }
    else if (idf_find(id)) {
        sprintf(str_err,"ERROR: identifier `%s` already exists",id);
        yyerror(str_err);
        return 0;
    }
    else {
        new_idf = (idf_t*)malloc(sizeof(idf_t));
        new_idf->name = strdup(id); //do not strdup, bison did
        new_idf->ival = MAX_IDF-idf_empty;
        idf_table[MAX_IDF-idf_empty] = new_idf;
        idf_empty--;
        return 1;
    }
}

void idf_set_type(data_t *type) {
    if (type) {
        idf_data_type = type;
    }
    else {
        idf_init();
        yyerror("ERROR: data type for record field is not defined");
    }
}

void idf_addto_record() {
    int i;

    usr_datatype->is = TYPE_RECORD;

    if ((usr_datatype->field_num + MAX_IDF - idf_empty)<=MAX_FIELDS) {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            //if no identifiers, we don't reach here
            if (check_for_id_in_datatype(usr_datatype,idf_table[i]->name)<0) {
                usr_datatype->field_name[usr_datatype->field_num] = idf_table[i]->name;
                usr_datatype->field_datatype[usr_datatype->field_num] = idf_data_type;
                usr_datatype->field_offset[usr_datatype->field_num] = usr_datatype->memsize; //memsize so far
                usr_datatype->field_num++;
                usr_datatype->memsize += idf_data_type->memsize;
            }
            else {
                sprintf(str_err,"ERROR: '%s' declared previously in '%s' record type, ignoring",idf_table[i]->name,usr_datatype->data_name);
                yyerror(str_err);
            }
        }
    }
    else {
        yyerror("ERROR: Too much fields in record type");
    }
    idf_init();
}

void idf_init() {
    int i;
    idf_data_type = NULL;
    idf_empty = MAX_IDF;
    for (i=0;i<MAX_IDF;i++) {
        if (idf_table[i]!=NULL) {
            if (idf_free_memory) {
                free(idf_table[i]->name);
            }
            free(idf_table[i]);
            idf_table[i] = NULL;
        }
        else {
            break;
        }
    }
    idf_free_memory = 0;
}

void init_symbol_table() {
    int i;

#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
    printf("Initializing symbol table.. ");
#endif

    idf_free_memory = 0; //at the beggining there is no memory
    idf_init();

    usr_datatype = (data_t*)malloc(sizeof(data_t));

    void_datatype = (data_t*)malloc(sizeof(struct data_t));
    void_datatype->is = TYPE_VOID;
    void_datatype->def_datatype = void_datatype;
    void_datatype->data_name = "__void_datatype__";
    void_datatype->memsize = 0; //no return type,so no size of return type :)

    lost_var = (var_t*)malloc(sizeof(var_t));
    lost_var->id_is = ID_LOST;
    lost_var->name = "V__dummy_lost_variable";
    lost_var->datatype = void_datatype;
    lost_var->scope = get_current_scope();
    lost_var->ival = 0;
    lost_var->fval = 0;
    lost_var->cval = 0;
    lost_var->Lvalue = (mem_t*)malloc(sizeof(mem_t));
    lost_var->Lvalue->content_type = PASS_VAL;
    lost_var->Lvalue->offset_expr = NULL;
    lost_var->Lvalue->seg_offset = 0;
    lost_var->Lvalue->segment = MEM_STACK;
    lost_var->Lvalue->size = 0;

    sm_empty = MAX_SYMBOLS;
    sm_table = sm_array;

    sm_scope = 0; //main scope

    root_scope_with = NULL;
    tail_scope_with = NULL;

    sem_main_program = sm_insert("");
    sem_main_program->id_is = ID_PROGRAM_NAME;
    main_program = (func_t*)malloc(sizeof(func_t));
    main_program->func_name = sem_main_program->name;
    sem_main_program->subprogram = main_program;

    scope_stack[0].scope_type = SCOPE_MAIN;
    scope_stack[0].scope_owner = main_program;
    scope_stack[0].start_index = 0;
    scope_stack[0].lost_symbols = (char**)malloc(MAX_LOST_SYMBOLS*sizeof(char*));
    scope_stack[0].lost_symbols_empty = MAX_LOST_SYMBOLS;

    for (i=0;i<MAX_LOST_SYMBOLS;i++) {
        scope_stack[0].lost_symbols[i] = NULL;
    }

    for (i=1;i<MAX_SCOPE+1;i++) {
        scope_stack[i].scope_type = SCOPE_IGNORE;
        scope_stack[i].scope_owner = NULL;
        scope_stack[i].lost_symbols = NULL;
    }

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

#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
    printf("OK\n");
#endif
}

void set_main_program_name(char *name) {
    free(sem_main_program->name);
    sem_main_program->name = strdup(name);
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
            return NULL;
        }
    }
    return NULL;
}

sem_t *sm_insert(const char *id) {
    //sets only the name of the symbol
    sem_t *new_sem;

    new_sem = sm_find(id);
    if ((!new_sem || new_sem->scope!=sm_scope || root_scope_with) && sm_empty) {
        new_sem = (sem_t*)malloc(sizeof(sem_t));
        new_sem->name = strdup(id);
        new_sem->scope = sm_scope;
        new_sem->index = MAX_SYMBOLS-sm_empty;
        sm_table[MAX_SYMBOLS-sm_empty] = new_sem;
        sm_empty--;
        //printf("__symbol__ `%s` inserted :empty=%d\n",id,sm_empty);
        return new_sem;
    }
    else if (!sm_empty) {
        yyerror("ERROR: symbol table is full, cannot insert new symbols.");
        exit(EXIT_FAILURE);
    }
    else {
        sprintf(str_err,"ERROR: `%s` already declared in this scope",id);
        yyerror(str_err);
        return NULL;
    }
}

void sm_remove(char *id) {
    int i;
    sem_t *symbol;
    func_t *scope_owner;

    symbol = sm_find(id);

    if (!symbol) {
        yyerror("ERROR: trying to remove null symbol from symbol table (debugging info)");
        return;
    }

    switch (symbol->id_is) {
    case ID_VAR_GUARDED:
    case ID_RETURN:
    case ID_VAR:
        //INFO: do not remove the lvalue, it is used from the ir.c //BUG FIXED
        //free(symbol->var->Lvalue);
        //do not break here
    case ID_VAR_PTR:
        free(symbol->var);
        //do not free the Lvalue, we need it for subprogram calls
        break;
    case ID_LOST:
    case ID_PROGRAM_NAME:
        break;
    case ID_STRING:
        free(symbol->var->cstr);
        break;
    case ID_CONST:
        break;
    case ID_FUNC:
    case ID_PROC:
        for (i=0;i<symbol->subprogram->param_num;i++) {
            free(symbol->subprogram->param[i]->name);
            free(symbol->subprogram->param[i]);
        }
        free(symbol->subprogram);
        break;
    case ID_TYPEDEF:
        for (i=0;i<symbol->comp->field_num;i++) {
            free(symbol->comp->field_name[i]);
        }
        break;
    case ID_FORWARDED_FUNC:
    case ID_FORWARDED_PROC:
        scope_owner = get_current_scope_owner();
        sprintf(str_err,"ERROR: subprogram '%s' without body in scope of %s.",id,scope_owner->func_name);
        yyerror(str_err);
        break;
    }
    free(symbol->name);
    free(symbol);
    symbol = NULL;
    sm_empty++;
}

void start_new_scope(scope_type_t scope_type, func_t *scope_owner) {
    int i;

    if (scope_type==SCOPE_IGNORE || scope_type==SCOPE_WITH_STATEMENT) {
        yyerror("INTERNAL_ERROR: start new scope with invalid scope flag");
        exit(EXIT_FAILURE);
    }
    else if (sm_scope<MAX_SCOPE) { //SCOPE_PROC, SCOPE_FUNC
        sm_scope++;
        scope_stack[sm_scope].scope_type = scope_type;
        scope_stack[sm_scope].scope_owner = scope_owner;
        scope_stack[sm_scope].lost_symbols_empty = MAX_LOST_SYMBOLS;
        scope_stack[sm_scope].lost_symbols = (char**)malloc(MAX_LOST_SYMBOLS*sizeof(char*));

        for (i=0;i<MAX_LOST_SYMBOLS;i++) {
            scope_stack[sm_scope].lost_symbols[i] = NULL;
        }

        if (sm_empty==MAX_SYMBOLS) {
            scope_stack[sm_scope].start_index = 0;
        }
        else {
            scope_stack[sm_scope].start_index = sm_table[MAX_SYMBOLS-sm_empty-1]->index + 1;
        }
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

    scope_stack[sm_scope].scope_type = SCOPE_IGNORE;
    sm_scope--;
#if SYMBOL_TABLE_DEBUG_LEVEL >= 1
    printf("__close_current_scope_%d\n",sm_scope);
#endif
}

int get_current_scope() {
    return sm_scope;
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

void declare_consts(char *id,expr_t *l) {
    sem_t *sem;
    var_t *new_var;
    char *lost_id;

    if (!l) {
        yyerror("UNEXPECTED_ERROR: 11");
        exit(EXIT_FAILURE);
    }
    if (!l->datatype) {
        yyerror("UNEXPECTED_ERROR: 12");
        exit(EXIT_FAILURE);
    }

    if (l->expr_is!=EXPR_HARDCODED_CONST) {
        lost_id = sm_find_lost_symbol(id);
        if (!lost_id) {
            sm_insert_lost_symbol(id);
            //sprintf(str_err,"ERROR: undeclared symbol '%s'",id);
            //yyerror(str_err);
        }
        sprintf(str_err,"ERROR: non constant value in constant declaration of '%s'",id);
        yyerror(str_err);
        return;
    }

    if (!TYPE_IS_STANDARD(l->datatype) && l->datatype->is!=TYPE_ENUM) {
        //we have only one value so it cannot be a subset
        yyerror("ERROR: invalid nonstandard datatype of constant declaration");
        lost_id = sm_find_lost_symbol(id);
        if (!lost_id) {
            sm_insert_lost_symbol(id);
            //sprintf(str_err,"ERROR: undeclared symbol '%s'",id);
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
        sprintf(str_err,"ERROR: '%s' already declared",id);
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
            new_sem = sm_insert(idf_table[i]->name);
            if (new_sem) {
                new_sem->id_is = ID_VAR;
                new_sem->var = (var_t*)malloc(sizeof(var_t));
                new_sem->var->id_is = ID_VAR;
                new_sem->var->datatype = type;
                new_sem->var->name = new_sem->name;
                new_sem->var->Lvalue = mem_allocate_symbol(type);
            }
            else {
                sprintf(str_err,"ERROR: '%s' already declared",idf_table[i]->name);
                yyerror(str_err);
            }
        }
    }
    else {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            lost_id = sm_find_lost_symbol(idf_table[i]->name);
            if (!lost_id) {
                sm_insert_lost_symbol(idf_table[i]->name);
                //sprintf(str_err,"ERROR: undeclared symbol '%s'",id);
                //yyerror(str_err);
            }
        }
#if BISON_DEBUG_LEVEL >= 1
        yyerror("ERROR: trying to declare variable(s) of unknown datatype (debugging info)");
#endif
    }

    idf_free_memory = 1;
    idf_init();
}

void sm_insert_lost_symbol(char *id) {
    char **pool;
    char *tmp;
    int index;

    tmp = sm_find_lost_symbol(id);
    if (tmp) {
        return;
    }

    pool = scope_stack[sm_scope].lost_symbols;
    index = scope_stack[sm_scope].lost_symbols_empty;

    if (index>0) {
        tmp = strdup(id);
        pool[MAX_LOST_SYMBOLS - index] = tmp;
        scope_stack[sm_scope].lost_symbols_empty--;
    }
    else {
        sprintf(str_err,"FATAL_ERROR: reached maximun lost symbols from current scope");
        yyerror(str_err);
        exit(EXIT_FAILURE);
    }
}

char *sm_find_lost_symbol(char *id) {
    char **pool;
    int index;
    int i;

    pool = scope_stack[sm_scope].lost_symbols;
    index = MAX_LOST_SYMBOLS - scope_stack[sm_scope].lost_symbols_empty;

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
        sprintf(str_err,"ERROR: null variable in with_statement (debugging info)");
        yyerror(str_err);
        return;
#endif
    }

    if (var->datatype->is!=TYPE_RECORD) {
        yyerror("ERROR: the type of variable is not a record");
        return;
    }

    if (!root_scope_with) { //this is the first with_statement
        root_scope_with = (with_stmt_scope_t*)malloc(sizeof(with_stmt_scope_t));
        root_scope_with->prev = root_scope_with;
        root_scope_with->next = NULL;
        tail_scope_with = root_scope_with;
    }
    else { //this is a nested with_statement
        new_with_stmt_scope = (with_stmt_scope_t*)malloc(sizeof(with_stmt_scope_t));
        new_with_stmt_scope->prev = tail_scope_with;
        new_with_stmt_scope->next = NULL;
        tail_scope_with->next = new_with_stmt_scope;
        tail_scope_with = new_with_stmt_scope;
    }

    //check for possible conflicts between nested with_statements
    for(with_scope=root_scope_with;with_scope;with_scope=with_scope->next) {
        for(i=0;i<var->datatype->field_num;i++) {
            if (check_for_id_in_datatype(with_scope->type,var->datatype->field_name[i])>=0) {
                sprintf(str_err,"ERROR: conflict in nested with_statement of type '%s' inside type '%s',\n\t both record datatypes have the name '%s' for element"
                        ,var->datatype->data_name,with_scope->type->data_name,var->datatype->field_name[i]);
                yyerror(str_err);
                tail_scope_with->type = NULL;
                return; //do not insert any record element
            }
        }
    }
    //no conflicts, continue

    //set the datatype of the with statement
    tail_scope_with->type = var->datatype;

    //insert the record elements
    for (i=0;i<var->datatype->field_num;i++) {
        new_sem = sm_insert(var->datatype->field_name[i]);
        new_sem->var = refference_to_record_element(var,var->datatype->field_name[i]);
    }
}

void close_last_opened_with_statement_scope() {
    int i;

    //with scopes close backwards (the last with scope closes first)

    //reminder: recursive data types are forbidden

    if (!root_scope_with || !tail_scope_with) {
        //root_scope_with and tail_scope_with are not initialised,
        //someone called this function without calling the start_new_with_statement_scope() first
        yyerror("INTERNAL_ERROR: trying to close null with statement scope (debugging info)");
        exit(EXIT_FAILURE);
    }

    if (tail_scope_with->type) {
        //remove backwards to find the symbol entry faster
        for (i=tail_scope_with->type->field_num-1;i>=0;i--) {
            sm_remove(tail_scope_with->type->field_name[i]);
        }
    }
    //else
    //there was a conflict between nested with_statements and nothing inserted
    //so nothing to remove, just close the with_scope

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

sem_t *reference_to_forwarded_function(char *id) {
    //this function was declared before in this scope and now we define it, stack is already configured
    sem_t *sem_2;

    sem_2 = sm_find(id);
    if (sem_2) {
        if (sem_2->id_is==ID_FUNC) {
            yyerror("ERROR: Multiple body definitions for a function");
        }
        else if (sem_2->id_is==ID_PROC) {
            yyerror("ERROR: Multiple definition of procedure. id is a procedure not a function and is already declared");
        }
        else if (sem_2->id_is==ID_FORWARDED_FUNC && sem_2->id_is==ID_FORWARDED_PROC) {
            yyerror("ERROR: id is not declared as a subprogram");
        }
        else if (sem_2->id_is==ID_FORWARDED_PROC) {
            yyerror("ERROR: id is a procedure not a function, and procedures cannot be forwarded");
        }
        else { //everything is OK, try to open new scope
            //try at first to open the new scope, to avoid false error messages if the function name does not exist in symbol table
            start_new_scope(SCOPE_FUNC,sem_2->subprogram);
            return sem_2;
        }
    }
    else {
        if (!sm_find_lost_symbol(id)) {
            sm_insert_lost_symbol(id);
            sprintf(str_err,"ERROR: invalid forwarded function. '%s' not declared before in this scope",id);
            yyerror(str_err);
        }
    }
    return NULL;
}

var_t *refference_to_variable_or_enum_element(char *id) {
    sem_t *sem_1;
    var_t *new_enum_const;
    func_t *scope_owner;
    char *lost_id;

    sem_1 = sm_find(id);
    if (sem_1) {
        if (sem_1->id_is==ID_VAR || sem_1->id_is==ID_VAR_PTR || sem_1->id_is==ID_CONST) {
            return sem_1->var;
        }
        else if (sem_1->id_is==ID_FORWARDED_FUNC) { //we are inside a function declaration
            //the name of the function acts like a variable
            scope_owner = get_current_scope_owner();
            if (scope_owner==main_program) {
                yyerror("ERROR: the main program does not return any value");
                return lost_var_reference();
            }
            return sem_1->var;
        }
        else if (sem_1->id_is==ID_TYPEDEF && sem_1->comp->is==TYPE_ENUM) {
            //ALLOW this if only enumerations and subsets can declare an iter_space
            if (strcmp(sem_1->comp->data_name,id)==0) {
                sprintf(str_err,"ERROR: '%s' is the name of the enumeration, expected only an element",id);
                yyerror(str_err);
                return lost_var_reference();
            }
            //the ID is an enumeration element
            new_enum_const = (var_t*)malloc(sizeof(var_t));
            new_enum_const->id_is = ID_CONST;
            new_enum_const->datatype = sem_1->comp;
            new_enum_const->name = sem_1->comp->field_name[enum_num_of_id(sem_1->comp,id)];
            new_enum_const->Lvalue = NULL;
            new_enum_const->cond_assign = NULL;
            new_enum_const->ival = enum_num_of_id(sem_1->comp,id);
            //do not set the scope, this is not a variable
            return new_enum_const;
        }
        else {
            sprintf(str_err,"ERROR: id '%s' exists but it is not a variable or constant",id);
            yyerror(str_err);
            return lost_var_reference();
        }
    }
    else {
        lost_id = sm_find_lost_symbol(id);
        if (!lost_id) {
            sm_insert_lost_symbol(id);
            sprintf(str_err,"ERROR: undeclared symbol '%s'",id);
            yyerror(str_err);
        }
        return lost_var_reference();
    }
}

var_t *refference_to_record_element(var_t *v, char *id) {
    //this is a struct, we take ID's type
    int elem_num;
    var_t *new_var;
    expr_t *offset_expr;
    expr_t *final_expr_offset;
    mem_t *new_mem;

    if (v) {
        if (v->id_is==ID_VAR || v->id_is==ID_VAR_PTR) {
            if (v->datatype->is==TYPE_RECORD) {
                elem_num = check_for_id_in_datatype(v->datatype,id);
                if (elem_num>=0) {
                    //element's position does not change so it is represented as a hardcoded constant
                    offset_expr = expr_from_hardcoded_int(v->datatype->field_offset[elem_num]);
                    final_expr_offset = expr_relop_equ_addop_mult(v->Lvalue->offset_expr,OP_PLUS,offset_expr);

                    new_mem = (mem_t*)malloc(sizeof(struct mem_t));
                    new_mem->direct_register_number = v->Lvalue->direct_register_number;
                    new_mem->segment = v->Lvalue->segment;
                    new_mem->seg_offset = v->Lvalue->seg_offset;
                    new_mem->offset_expr = final_expr_offset;
                    new_mem->content_type = v->Lvalue->content_type;
                    new_mem->size = v->datatype->field_datatype[elem_num]->memsize;

                    new_var = (var_t*)malloc(sizeof(var_t));
                    new_var->id_is = ID_VAR;
                    new_var->datatype = v->datatype->field_datatype[elem_num];
                    //new_var->name = v->datatype->field_name[elem_num]; //BUG strdup the name because we free() it later
                    new_var->name = strdup(v->datatype->field_name[elem_num]);
                    new_var->scope = v->scope;
                    new_var->Lvalue = new_mem;

                    return new_var;
                }
                else {
                    sprintf(str_err,"ERROR: no element named '%s' in record type",id);
                    yyerror(str_err);
                    return lost_var_reference();
                }
            }
            else {
                yyerror("ERROR: type of variable is not a record");
                return lost_var_reference();
            }
        }
        else if (v->id_is==ID_LOST) {
            return v; //avoid unreal error messages
        }
        else {
            sprintf(str_err,"ERROR: id '%s' is not a variable",v->name);
            yyerror(str_err);
            return lost_var_reference();
        }
    }
    else {
        yyerror("INTERNAL_ERROR: 43");
        exit(EXIT_FAILURE);
    }
}

int enum_num_of_id(const data_t *data,const char *id) {
    int i=0;
    //if this function is called, id exists for sure
    while (strcmp(data->field_name[i],id)!=0) {
        i++;
    }
    return data->enum_num[i];
}

data_t *close_datatype_start_new() {
    data_t *old_usr_datatype;

    old_usr_datatype = usr_datatype;
    usr_datatype = (data_t*)malloc(sizeof(data_t));
    return old_usr_datatype;
}

data_t *close_array_type(data_t *def_type) {
    //there is at least one dimension
    int i;
    //    int num_of_elements = 0;
    //
    //    for(i=0;i<usr_datatype->field_num;i++) {
    //      num_of_elements += usr_datatype->dim[i]->range;
    //    }

    //    int n1,n2;

    if (def_type) {
        usr_datatype->def_datatype = def_type;
        //usr_datatype->memsize = num_of_elements * usr_datatype->datadef_type->memsize;

        usr_datatype->memsize = usr_datatype->dim[0]->relative_distance * usr_datatype->dim[0]->range * usr_datatype->def_datatype->memsize;
        return close_datatype_start_new();
    }
    else {
        for(i=0;i<usr_datatype->field_num;i++) {
            free(usr_datatype->dim[i]);
            usr_datatype->dim[i] = NULL;
        }
        return NULL;
    }
}

data_t *close_set_type(data_t *type) {
    //memsize is a power of 2 depending of the elements' data type
    int size=0;
    int bytes;

    usr_datatype->is = TYPE_SET;
    size = calculate_number_of_set_elements(type);
    if (size) {
        usr_datatype->def_datatype = type;
        bytes = size/8;
        if (size%8) {
            bytes++;
        }
        usr_datatype->memsize = bytes;
        return close_datatype_start_new();
    }
    else {
        yyerror("ERROR: cannot define set type for this data type");
        return NULL;
    }
}

data_t *close_record_type() {
    //memsize of new record data type is already set
    //see idf_addto_record() function

    if (usr_datatype->field_num>0) {
        return close_datatype_start_new();
    }
    else {
        //posibly parse errors
        //record datatypes must have at least one element
        return NULL;
    }
}

/** Memory size could be relative to enum's (subset's) range
 * but we use the same size with integers. we play safe here,
 * for binary compatibility between programs (database
 * programs for example). Some programs could use integers
 * and some could use enums or subsets for the same task.
 * This goes to far, but whatever :)
 */
data_t *close_enum_type() {
    int i;
    char *id;
    for (i=0;i<MAX_IDF-idf_empty;i++) {
        id = idf_table[i]->name;
        if (sm_find(id)) {
            sprintf(str_err,"ERROR: Identifier `%s` already exists in this scope",id);
            yyerror(str_err);
            usr_datatype->memsize = MEM_SIZEOF_INT;
            return close_datatype_start_new();
        }
    }

    usr_datatype->is = TYPE_ENUM;
    usr_datatype->def_datatype = SEM_INTEGER;
    if ((MAX_IDF - idf_empty)<=MAX_FIELDS) {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            usr_datatype->field_name[i] = idf_table[i]->name;
            usr_datatype->enum_num[i] = idf_table[i]->ival;
            usr_datatype->field_num++;
        }
        idf_init();
    }
    else {
        yyerror("ERROR: Too much identifiers in enum type");
    }

    usr_datatype->memsize = MEM_SIZEOF_INT;
    return close_datatype_start_new();
}

data_t *close_subset_type(expr_t *l1, expr_t *l2) {
    //this function defines a new type which is a subset of another
    //if everything ok set the *record pointer
    int i;

    if (!l1 || !l2) {
        printf("INTERNAL ERROR: null expression in subset definition\n");
        return NULL;
    }

    if (!TYPE_IS_SUBSET_VALID(l1->datatype) || !TYPE_IS_SUBSET_VALID(l2->datatype)) {
        yyerror("ERROR: invalid datatype in subset declaration");
        return NULL;
    }

    if (l1->datatype!=l2->datatype) {
        yyerror("ERROR: in subset definition, bounds have different data types");
        return NULL;
    }

    if (l1->expr_is!=EXPR_HARDCODED_CONST || l2->expr_is!=EXPR_HARDCODED_CONST) {
        yyerror("ERROR: subset limits MUST be constants");
        return NULL;
    }

    if (l2->ival - l1->ival + 1<=0 || l2->ival - l1->ival + 1>MAX_FIELDS) {
        yyerror("ERROR: subset limits are incorrect");
        return NULL;
    }

    //type of expressions is either SEM_CHAR or SEM_INTEGER or an enumeration
    usr_datatype->is = TYPE_SUBSET;
    usr_datatype->def_datatype = l1->datatype;

    usr_datatype->field_num = l2->ival - l1->ival +1;
    for (i=0;i<usr_datatype->field_num;i++) {
        usr_datatype->enum_num[i] = l1->ival + i;
    }
    usr_datatype->memsize = l1->datatype->memsize;
    return close_datatype_start_new();
}

void add_dim_to_array_type(dim_t *dim) {
    int i;

    usr_datatype->is = TYPE_ARRAY;

    //this is the last dimension so far
    dim->relative_distance = 1;

    if (usr_datatype->field_num<MAX_ARRAY_DIMS) {
        //update dim_size in memory
        for (i=usr_datatype->field_num-1;i>=0;i--) {
            usr_datatype->dim[i]->relative_distance *= dim->range;
        }
        usr_datatype->dim[usr_datatype->field_num] = dim;
        usr_datatype->field_num++;
    }
    else {
        //yyerror("ERROR: too much dimensions");
    }
}

void make_type_definition(char *id, data_t *type) {
    sem_t *sem_1;
    if (type!=NULL) {
        if (!sm_find(id)) {
            sem_1 = sm_insert(id);
            //set semantics to symbol
            sem_1->id_is = ID_TYPEDEF;
            sem_1->comp = type;
            sem_1->comp->data_name = sem_1->name;
        }
        else {
            yyerror("ERROR: id type declaration already exists");
        }
    }
    else {
        yyerror("ERROR: data type NOT defined (debugging info)");
    }
}

int calculate_number_of_set_elements(data_t *type) {
    //returns the size of the new SET type, or zero if
    //we cannot create it
    if (!type) {
        return 0;
    }
    else if (type->is==TYPE_CHAR) {
        return MAX_SET_ELEM;
    }
    else if (type->is==TYPE_BOOLEAN) {
#warning what is the point of a boolean set??
        return 2; //number of elements
    }
    else if (type->is==TYPE_ENUM || type->is==TYPE_SUBSET) {
        if (type->field_num <= MAX_SET_ELEM) {
            return type->field_num;
        }
    }
    return 0;
}

int check_for_id_in_datatype(data_t *datatype,const char *id) {
    //checks if id exists in the given composite type and returns its index
    int i;
    for (i=0;i<datatype->field_num;i++) {
        if (!datatype->field_name[i]) {
            return -1;
        }
        else if (strcmp(datatype->field_name[i],id)==0) {
            return i;
        }
    }
    return -1;
}

void protect_guard_var(char *id) {
    sem_t *sem_2;

    sem_2 = sm_find(id);
    if (sem_2) {
        if (sem_2->id_is==ID_VAR) {
            if (sem_2->var->datatype->is==TYPE_INT) {
                sem_2->var->id_is = ID_VAR_GUARDED;
            }
            else {
                sprintf(str_err,"ERROR: control variable '%s' must be integer",id);
                yyerror(str_err);
            }
        }
        else if (sem_2->id_is==ID_VAR_GUARDED) {
            sprintf(str_err,"ERROR: variable '%s' already controls a for statement",id);
            yyerror(str_err);
        }
        else {
            sprintf(str_err,"ERROR: invalid reference to '%s', expected variable",id);
            yyerror(str_err);
        }
    }
    //else
    //nothing, error is printed from sm_insert
    //yyerror("ERROR: the name of the control variable is declared before in this scope");
}

void unprotect_guard_var(char *id) {
    sem_t *sem_2;

    sem_2 = sm_find(id);
    if (sem_2) {
        if (sem_2->id_is==ID_VAR_GUARDED) {
            sem_2->var->id_is = ID_VAR;
        }
    }
}

var_t *lost_var_reference() {
    return lost_var;
}
