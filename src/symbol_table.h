#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#include "semantics.h"

//ARRAY implementation of symbol table
#define MAX_SYMBOLS 256
#define MAX_LOST_SYMBOLS 32
extern int sm_empty;

//general symbol
typedef struct sem_t {
    idt_t id_is;         //object type
    var_t *var;          //variable
    func_t *subprogram;  //subprogram/module
    data_t *comp;        //composite type
    char *name;          //name of symbol (unique global pointer)
    func_t *scope;       //parent module of declaration
    int index;           //relative position in symbol table (array implementation specific)
} sem_t;

//low level (assembler) headers
extern const char * DOT_BYTE;    //.data
extern const char * DOT_WORD;    //.word
extern const char * DOT_SPACE;   //.space
extern const char * DOT_ASCIIZ;  //.asciiz

void init_symbol_table();
func_t *create_main_program(char *name);

//search token by name
//search the scopes backwards, this IS CRITICAL for the whole implementation
sem_t *sm_find(const char *id);

//insert token as ID_TYPE
//insert symbol to the current scope, sets only the name
sem_t *sm_insert(char *id, idt_t ID_TYPE);

//remove token by name
//remove this symbol from the most nested scope it is found
void sm_remove(char *id);

//declaration helpers
void declare_consts(char *id, expr_t *l);
void declare_vars(data_t *type);
void declare_formal_parameters(func_t *subprogram);

//bad token handlers,
//primarilly avoid duplicate error messages
void sm_insert_lost_symbol(char *id, const char *error_msg);
char *sm_find_lost_symbol(const char *id);

//use this for bad variable declaration/use, to ignore further
//analysis of the parent expression/statement
var_t *lost_var_reference();

#endif
