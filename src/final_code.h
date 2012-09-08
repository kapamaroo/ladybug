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

#if (USE_PSEUDO_INSTR_LA==1)
reg_t *base_addr_is_alive(ir_node_t *node);
void mark_addr_alive(ir_node_t *node);
#endif

void print_assembly();

typedef void (*pass_ptr_t)(instr_t *);

//passes handler
void run_pass(pass_ptr_t do_pass);
void remove_move_chains(instr_t *instr);
void combine_immediates(instr_t *instr);

//passes
void reuse_and_rename(instr_t *instr);

#endif
