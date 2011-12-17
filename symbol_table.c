#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "build_flags.h"
#include "semantics.h"
#include "bison.tab.h"
#include "symbol_table.h"
#include "scope.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "mem_reg.h"
#include "err_buff.h"

//ARRAY implementation of symbol table
int sm_empty;
sem_t *sm_array[MAX_SYMBOLS];
sem_t **sm_table;
//sem_t *sm_table[MAX_SYMBOLS];

sem_t *sem_main_program;
func_t *main_program;

sem_t *sem_INTEGER;
sem_t *sem_REAL;
sem_t *sem_BOOLEAN;
sem_t *sem_CHAR;

data_t *VIRTUAL_STRING_DATATYPE;

idf_t *idf_table[MAX_IDF];
data_t *idf_data_type;
int idf_empty = MAX_IDF;
int idf_free_memory;  //flag for idf_init() to free the memory of the symbols because sm_insert() copied called strdup

data_t *usr_datatype; //new user defined datatype
var_t *lost_var;

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
    data_t *void_datatype; //datatype of lost symbols

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
    lost_var->scope = &scope_stack[0]; //lost symbols are adopted by main_program_scope
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
    sem_t *existing_sem;
    sem_t *new_sem;

    existing_sem = sm_find(id);
    if ((!existing_sem || existing_sem->scope!=get_current_scope() || root_scope_with) && sm_empty) {
        new_sem = (sem_t*)malloc(sizeof(sem_t));
        new_sem->name = strdup(id);
        new_sem->scope = &scope_stack[sm_scope];
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
        free(symbol->var);
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

        new_sem->var = new_var;
    }
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

sem_t *reference_to_forwarded_function(char *id) {
    //this function was declared before in this scope and now we define it, stack is already configured
    sem_t *sem_2;

    sem_2 = sm_find(id);
    if (sem_2) {
        if (sem_2->id_is==ID_FUNC) {
            yyerror("ERROR: Multiple body definitions for a function");
        }
        else if (sem_2->id_is==ID_PROC) {
            yyerror("ERROR: id is a procedure not a function and is already declared");
        }
        else if (sem_2->id_is==ID_FORWARDED_PROC) {
            yyerror("ERROR: id is a procedure not a function, and procedures cannot be forwarded");
        }
        else if (sem_2->id_is!=ID_FORWARDED_FUNC) {
            yyerror("ERROR: id is not declared as a subprogram");
        }
        else {
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
        if (sem_1->id_is==ID_VAR || sem_1->id_is==ID_CONST) {
            return sem_1->var;
        }
        else if (sem_1->id_is==ID_FORWARDED_FUNC) { //we are inside a function declaration
            //the name of the function acts like a variable
            scope_owner = get_current_scope_owner();
            if (scope_owner==main_program) {
                yyerror("ERROR: the main program does not return any value");
                return lost_var_reference();
            }
            return sem_1->subprogram->return_value;
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

var_t *refference_to_array_element(var_t *v, expr_list_t *list) {
    //reference to array
    expr_t *relative_offset_expr;
    expr_t *final_offset_expr;
    expr_t *cond_expr;
    mem_t *new_mem;
    var_t *new_var;

    if (!list) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("UNEXPECTED_ERROR: null expr_list for array refference (debugging info)");
#endif
        exit(EXIT_FAILURE);
    }

    if (v) {
        if (v->id_is==ID_VAR) {
            if (v->datatype->is==TYPE_ARRAY) {
                if (valid_expr_list_for_array_reference(v->datatype,list)) {
                    relative_offset_expr = make_array_refference(list,v->datatype);
                    cond_expr = make_array_bound_check(list,v->datatype);
                }
                else {
                    relative_offset_expr = expr_from_hardcoded_int(0);
                    cond_expr = expr_from_hardcoded_boolean(0);
                }
                final_offset_expr = expr_relop_equ_addop_mult(v->Lvalue->offset_expr,OP_PLUS,relative_offset_expr);

                //we start from the variable's mem position and we add the offset from there
                new_mem = (mem_t*)malloc(sizeof(mem_t));
                new_mem = (mem_t*)memcpy(new_mem,v->Lvalue,sizeof(mem_t));
                new_mem->offset_expr = final_offset_expr;
                new_mem->size = v->datatype->def_datatype->memsize;

                new_var = (var_t*)malloc(sizeof(var_t));
                new_var = (var_t*)memcpy(new_var,v,sizeof(var_t));
                //new_var->id_is = ID_VAR;
                new_var->datatype = v->datatype->def_datatype;
                //new_var->name = v->name;
                //new_var->scope = v->scope;
                new_var->Lvalue = new_mem;
                new_var->cond_assign = cond_expr;
                return new_var;
            }
            else {
                sprintf(str_err,"ERROR: variable '%s' is not an array",v->name);
                yyerror(str_err);
                return lost_var_reference(); //avoid unreal error messages
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
        yyerror("UNEXPECTED_ERROR: 42");
        exit(EXIT_FAILURE);
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
        if (v->id_is==ID_VAR) {
            if (v->datatype->is==TYPE_RECORD) {
                elem_num = check_for_id_in_datatype(v->datatype,id);
                if (elem_num>=0) {
                    //element's position does not change so it is represented as a hardcoded constant
                    offset_expr = expr_from_hardcoded_int(v->datatype->field_offset[elem_num]);
                    final_expr_offset = expr_relop_equ_addop_mult(v->Lvalue->offset_expr,OP_PLUS,offset_expr);

                    new_mem = (mem_t*)malloc(sizeof(mem_t));
                    new_mem = (mem_t*)memcpy(new_mem,v->Lvalue,sizeof(struct mem_t));
                    new_mem->offset_expr = final_expr_offset;
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
