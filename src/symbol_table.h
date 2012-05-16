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
    //scope_t *scope; //depth of declaration
    func_t *scope; //depth of declaration
    int index; //relative position in symbol table
    //mem_t *Lvalue;
} sem_t;

void init_symbol_table();
func_t *create_main_program(char *name);

sem_t *sm_find(const char *id); //search the scopes backwards, this IS CRITICAL for the whole implementation
sem_t *sm_insert(char *id, idt_t ID_TYPE); //insert symbol to the current scope, sets only the name
void sm_remove(char *id); //remove this symbol from the most nested scope it is found

void declare_consts(char *id,expr_t *l);
void declare_vars(data_t *type);
void declare_formal_parameters(func_t *subprogram);

void sm_insert_lost_symbol(char *id, const char *error_msg);
char *sm_find_lost_symbol(const char *id);

var_t *lost_var_reference();

#endif
