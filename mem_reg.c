#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "scope.h"
#include "symbol_table.h"
#include "mem_reg.h"

int gp;
//int sp; //the main program does not use the stack

void mem_reg_init() {
    gp = 0;
    //sp = 0;
}

mem_t* mem_allocate_symbol(data_t *d) {
    mem_t *m;
    mem_seg_t t;
    func_t *scope_owner;

    scope_owner = get_current_scope_owner();
    t = MEM_GLOBAL;
    if (scope_owner!=main_program) {
        t = MEM_STACK;
    }

    m = (mem_t*)malloc(sizeof(struct mem_t));
    m->segment = t;
    m->offset_expr = NULL;
    m->content_type = PASS_VAL;
    m->size = d->memsize;

    switch (t) {
    case MEM_GLOBAL:
        m->seg_offset = gp;
        gp += d->memsize;
        break;
    case MEM_STACK:
        //the main program does not use the stack
        m->seg_offset = scope_owner->stack_size;
        scope_owner->stack_size += d->memsize;
        break;
    case MEM_REGISTER:
        //not used here
        break;
    }
    return m;
}

mem_t *mem_allocate_string(char *string) {
    mem_t *m;

    m = (mem_t*)malloc(sizeof(struct mem_t));
    m->segment = MEM_GLOBAL;
    m->offset_expr = NULL;
    m->content_type = PASS_VAL;
    m->size = MEM_SIZEOF_CHAR*strlen(string);

    //all strings go to global pointer
    m->seg_offset = gp;
    gp += m->size;

    return m;
}

void configure_stack_size_and_param_lvalues(func_t *subprogram) {
    int i;
    mem_t *new_mem;

    //procedures have zero memsize so the stack_size initializes to zero for them
    subprogram->stack_size = STACK_INIT_SIZE; //standard independent stack size

    if (subprogram->return_value) {
        //add space for return_value if subprogram is a function
        subprogram->stack_size += subprogram->return_value->datatype->memsize;
    }

    //we do not put the variables in the stack here, just declare them in scope and allocate them
    for (i=0;i<subprogram->param_num;i++) {
        new_mem = (mem_t*)malloc(sizeof(mem_t));
        new_mem->offset_expr = NULL;
        new_mem->content_type = subprogram->param[i]->pass_mode;
        if (i<MAX_FORMAL_PARAMETERS_FOR_DIRECT_PASS) {
            new_mem->segment = MEM_REGISTER;
            new_mem->direct_register_number = i;
            new_mem->seg_offset = 0;
            new_mem->size = 0;
        }
        else {
            new_mem->segment = MEM_STACK;
            new_mem->direct_register_number = 0;
            new_mem->seg_offset = subprogram->stack_size; //stack_size so far
            new_mem->size = subprogram->param[i]->datatype->memsize;

            if (subprogram->param[i]->pass_mode==PASS_VAL) {
                subprogram->stack_size += subprogram->param[i]->datatype->memsize;
            }
            else { //PASS_REF
                subprogram->stack_size += SIZEOF_POINTER_TYPE; //sizeof memory address
            }
        }

        subprogram->param_Lvalue[i] = new_mem;
    }
}
