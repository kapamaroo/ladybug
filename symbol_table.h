#ifndef _SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "semantics.h"
#include "bison.tab.h"

#define MAIN_PROGRAM_SCOPE 0
#define MAX_IDF 128

//ARRAY implementation of symbol table
#define MAX_SYMBOLS 256
#define MAX_LOST_SYMBOLS 32
extern int sm_empty;

typedef struct idf_t {
    char *name;
    int ival; //used for enum type
} idf_t;

typedef struct with_stmt_scope_t {
    data_t *type; //record type of with statement
    //only if conflicts==0 it is allowed to close a with_scope, else conflicts--
    int conflicts; //(non negative) remember how many with_scopes failed to open due to element name conflicts
    struct with_stmt_scope_t *prev;
    struct with_stmt_scope_t *next;
} with_stmt_scope_t;

extern sem_t **sm_table;
extern int sm_scope;
extern scope_t scope_stack[MAX_SCOPE+1];
//scope 0 is the scope of the main program and if the
//symbol table is an array, it is the first element
//scope_stack[n] points to the first element of n-th scope

extern sem_t *sem_main_program;
extern func_t *main_program;

//pointers to standard type structures
extern sem_t *sem_INTEGER;
extern sem_t *sem_REAL;
extern sem_t *sem_BOOLEAN;
extern sem_t *sem_CHAR;

#define SEM_INTEGER (sem_INTEGER->comp)
#define SEM_REAL (sem_REAL->comp)
#define SEM_BOOLEAN (sem_BOOLEAN->comp)
#define SEM_CHAR (sem_CHAR->comp)

extern idf_t *idf_table[MAX_IDF];
extern data_t *idf_data_type;
extern int idf_empty;

extern data_t *void_datatype;

void idf_init();
idf_t *idf_find(const char *id);
int idf_insert(char *id);
void idf_set_type(data_t *type);
void idf_addto_record();

data_t *close_datatype_start_new();
data_t *close_array_type(data_t *def_type);
data_t *close_set_type(data_t *type);
data_t *close_record_type();
data_t *close_enum_type();
data_t *close_subset_type(expr_t *l1, expr_t *l2);

void add_dim_to_array_type(dim_t *dim);

int calculate_number_of_set_elements(data_t *type);
int check_for_id_in_datatype(data_t *datatype,const char *id);

void init_symbol_table();
void set_main_program_name(char *name);

sem_t *sm_find(const char *id); //search the scopes backwards, this IS CRITICAL for the whole implementation
sem_t *sm_insert(const char *id); //insert symbol to the current scope, sets only the name
void sm_remove(char *id); //remove this symbol from the most nested scope it is found

void start_new_scope(scope_type_t scope_type, func_t *scope_owner);
void close_current_scope();
void sm_clean_current_scope();

scope_t *get_current_scope();
int get_current_nesting();
func_t *get_current_scope_owner();
int get_nesting_of_var(var_t *v);

void protect_guard_var(char *id);
void unprotect_guard_var(char *id);

void start_new_with_statement_scope(var_t *var);
void close_last_opened_with_statement_scope();

void declare_consts(char *id,expr_t *l);
void declare_vars(data_t *type);

void sm_insert_lost_symbol(char *id);
char *sm_find_lost_symbol(char *id);

data_t *reference_to_typename(char *id);
var_t *refference_to_variable_or_enum_element(char *id);
var_t *refference_to_record_element(var_t *v, char *id);
sem_t *reference_to_forwarded_function(char *id);

var_t *lost_var_reference();
int enum_num_of_id(const data_t *data,const char *id);

#endif
