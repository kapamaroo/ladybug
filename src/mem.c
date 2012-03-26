#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "mem.h"
#include "scope.h"
#include "symbol_table.h"
#include "expr_toolbox.h"

static int gp = 0;
//main program does not use the stack

mem_t* mem_allocate_symbol(data_t *d) {
    mem_t *m;
    mem_seg_t t;
    func_t *scope_owner;

    scope_owner = get_current_scope_owner();
    t = MEM_GLOBAL;
    if (main_program && scope_owner!=main_program) {
        t = MEM_STACK;
    }

    m = (mem_t*)calloc(1,sizeof(struct mem_t));
    m->segment = t;
    m->offset_expr = expr_from_hardcoded_int(0);
    m->content_type = PASS_VAL;
    m->size = d->memsize;

    switch (t) {
    case MEM_GLOBAL:
        m->seg_offset = expr_from_hardcoded_int(gp);
        gp += d->memsize;
        break;
    case MEM_STACK:
        //the main program does not use the stack
        m->seg_offset = expr_from_hardcoded_int(scope_owner->stack_size);
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

    m = (mem_t*)calloc(1,sizeof(struct mem_t));
    m->segment = MEM_GLOBAL;
    m->offset_expr = expr_from_hardcoded_int(0);
    m->content_type = PASS_VAL;
    m->size = MEM_SIZEOF_CHAR*strlen(string);

    //all strings go to global pointer
    m->seg_offset = expr_from_hardcoded_int(gp);
    gp += m->size;

    return m;
}

mem_t *mem_allocate_return_value(func_t *subprogram) {
    mem_t *m;

    subprogram->stack_size += STACK_RETURN_VALUE_OFFSET;

    m = (mem_t*)calloc(1,sizeof(struct mem_t));
    m->segment = MEM_STACK;
    m->offset_expr = expr_from_hardcoded_int(0);
    m->seg_offset = expr_from_hardcoded_int(subprogram->stack_size);
    m->content_type = PASS_VAL;
    m->size = subprogram->return_value->datatype->memsize;

    subprogram->stack_size += m->size;

    return m;
}

void configure_stack_size_and_param_lvalues(func_t *subprogram) {
    int i;
    mem_t *new_mem;

    //we do not put the variables in the stack here, just declare them in scope and allocate them
    for (i=0;i<subprogram->param_num;i++) {
        new_mem = (mem_t*)calloc(1,sizeof(mem_t));
        new_mem->offset_expr = expr_from_hardcoded_int(0);
        new_mem->content_type = subprogram->param[i]->pass_mode;
        new_mem->segment = MEM_STACK;
        new_mem->direct_register_number = i;
        new_mem->seg_offset = expr_from_hardcoded_int(subprogram->stack_size); //stack_size so far

        if (subprogram->param[i]->pass_mode==PASS_VAL) {
            new_mem->size = subprogram->param[i]->datatype->memsize;
            subprogram->stack_size += subprogram->param[i]->datatype->memsize;
        }
        else { //PASS_REF
            new_mem->size = SIZEOF_POINTER_TYPE;
            subprogram->stack_size += SIZEOF_POINTER_TYPE; //sizeof memory address
        }

        subprogram->param_Lvalue[i] = new_mem;
    }
}
