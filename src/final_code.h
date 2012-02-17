#ifndef _FINAL_CODE_H
#define _FINAL_CODE_H

#include "ir.h"
#include "instruction_set.h"
#include "statements.h" //MAX_NUM_OF_MODULES

extern instr_t *final_tree[MAX_NUM_OF_MODULES];
extern int final_tree_empty;
extern instr_t *final_tree_current;

void init_final_code();
void new_final_tree();

void print_assembly();

#endif
