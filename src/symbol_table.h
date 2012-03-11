#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#include "semantics.h"

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

extern sem_t **sm_table;

//pointers to standard type structures
extern sem_t *sem_INTEGER;
extern sem_t *sem_REAL;
extern sem_t *sem_BOOLEAN;
extern sem_t *sem_CHAR;

#define SEM_INTEGER (sem_INTEGER->comp)
#define SEM_REAL (sem_REAL->comp)
#define SEM_BOOLEAN (sem_BOOLEAN->comp)
#define SEM_CHAR (sem_CHAR->comp)

void init_symbol_table();

sem_t *sm_find(const char *id); //search the scopes backwards, this IS CRITICAL for the whole implementation
sem_t *sm_insert(const char *id); //insert symbol to the current scope, sets only the name
void sm_remove(char *id); //remove this symbol from the most nested scope it is found

var_t *protect_guard_var(char *id);
void unprotect_guard_var(var_t *var);

void declare_consts(char *id,expr_t *l);
void declare_vars(data_t *type);

void configure_formal_parameters(param_list_t *list,func_t *func);
void declare_formal_parameters(func_t *subprogram);

void sm_insert_lost_symbol(const char *id);
char *sm_find_lost_symbol(char *id);

func_t *create_main_program(char *name);

var_t *lost_var_reference();

#endif
