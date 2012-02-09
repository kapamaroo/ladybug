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
instr_t *new_instruction(char *label,mips_instr_t *mips_instr);
instr_t *link_instructions(instr_t *child, instr_t *parent);

void print_assembly();

#endif
