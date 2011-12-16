#ifndef _FINAL_CODE_H
#define _FINAL_CODE_H

#include <stdio.h>
#include <stdlib.h>
#include "semantics.h"
#include "build_flags.h"
#include "ir.h"
#include "instructionset.h"

typedef struct code_rep{
    inst_t *node;
    struct code_rep *next;
    struct code_rep *begin;
    int branch; // 0 no branch 1 branch use begin;
}code_rep;


code_rep *global_begin;


code_rep *insert_in_code(code_rep *start,inst_t *node);
int travel_nodes(ir_node_t *start,code_rep *node);

#endif
