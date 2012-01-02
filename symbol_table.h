#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#include "semantics.h"

#define MAX_IDF 128

//ARRAY implementation of symbol table
#define MAX_SYMBOLS 256
#define MAX_LOST_SYMBOLS 32
extern int sm_empty;

typedef struct sem_t {
    idt_t id_is;
    var_t *var;
    func_t *subprogram;
    data_t *comp; //composite type
    char *name;
    scope_t *scope; //depth of declaration
    int index; //relative position in symbol table
    //mem_t *Lvalue;
} sem_t;

typedef struct idf_t {
    char *name;
    int ival; //used for enum type
} idf_t;

extern sem_t **sm_table;

//pointers to standard type structures
extern sem_t *sem_INTEGER;
extern sem_t *sem_REAL;
extern sem_t *sem_BOOLEAN;
extern sem_t *sem_CHAR;

extern data_t *VIRTUAL_STRING_DATATYPE;

#define SEM_INTEGER (sem_INTEGER->comp)
#define SEM_REAL (sem_REAL->comp)
#define SEM_BOOLEAN (sem_BOOLEAN->comp)
#define SEM_CHAR (sem_CHAR->comp)

extern idf_t *idf_table[MAX_IDF];
extern data_t *idf_data_type;
extern int idf_empty;

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
func_t *create_main_program(char *name);

sem_t *sm_find(const char *id); //search the scopes backwards, this IS CRITICAL for the whole implementation
sem_t *sm_insert(const char *id); //insert symbol to the current scope, sets only the name
void sm_remove(char *id); //remove this symbol from the most nested scope it is found

var_t *protect_guard_var(char *id);
void unprotect_guard_var(var_t *var);

void declare_consts(char *id,expr_t *l);
void declare_vars(data_t *type);
void declare_formal_parameters(func_t *subprogram);

void sm_insert_lost_symbol(char *id);
char *sm_find_lost_symbol(char *id);

data_t *reference_to_typename(char *id);
var_t *refference_to_variable_or_enum_element(char *id);
var_t *refference_to_array_element(var_t *v, expr_list_t *list);
var_t *refference_to_record_element(var_t *v, char *id);
sem_t *reference_to_forwarded_function(char *id);

func_t *find_subprogram(char *id);

var_t *lost_var_reference();
int enum_num_of_id(const data_t *data,const char *id);

#endif
