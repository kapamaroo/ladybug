#ifndef _MEM_H
#define _MEM_H

#include "semantics.h"
#include "mips32.h"
//#include "bison.tab.h"

extern int gp; //global static memory (heap)
//extern int sp; //dynamic memory (stack)

//size of standard types in memory in bytes
#define MEM_SIZEOF_CHAR 1
#define MEM_SIZEOF_INT 4
#define MEM_SIZEOF_REAL 4
#define MEM_SIZEOF_BOOLEAN MEM_SIZEOF_CHAR

//the stack size in the beggining of the call. NO RETURN VALUE, NO FORMAL PARAMETERS just the basic ABI
#define STACK_INIT_SIZE 0
#define STACK_RETURN_VALUE_OFFSET (STACK_INIT_SIZE)
#define MAX_FORMAL_PARAMETERS_FOR_DIRECT_PASS 4

//depends on machine type
#define SIZEOF_POINTER_TYPE MACHINE_ARCHITECTURE

void init_mem();
mem_t *mem_allocate_symbol(data_t *d);
mem_t *mem_allocate_string(char *string);

void configure_stack_size_and_param_lvalues(func_t *subprogram);

#endif
