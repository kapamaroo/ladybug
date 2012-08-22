#include <stdio.h>
#include <stdlib.h>

#include "build_flags.h"
#include "final_code.h"
#include "scope.h"
#include "symbol_table.h"
#include "instruction_set.h"
#include "reg.h"
#include "ir_parser.h"

#define COMMA() printf(", ")

instr_t *final_tree[MAX_NUM_OF_MODULES];
int final_tree_empty;

instr_t *final_tree_current;

#if (USE_PSEUDO_INSTR_LA==1)
#define MAX_ALIVE_ADDR 4
ir_node_t *alive_base_addr[MAX_ALIVE_ADDR];
int alive_addr_empty = MAX_ALIVE_ADDR;
#endif

struct alive_var {
    char *name;
    reg_t *reg;
};

#define MAX_ALIVE_VARS 4
struct alive_var alive_vars[MAX_ALIVE_VARS];
int alive_vars_empty = MAX_ALIVE_VARS;

void init_final_code() {
    int i;

    init_reg();

    final_tree_empty = MAX_NUM_OF_MODULES;

    for(i=0;i<MAX_NUM_OF_MODULES;i++)
        final_tree[i] = NULL;

#if (USE_PSEUDO_INSTR_LA==1)
    for (i=0; i<MAX_ALIVE_ADDR; i++)
        alive_base_addr[i] = NULL;
#endif

    final_tree_current = NULL;
}

#if (USE_PSEUDO_INSTR_LA==1)
reg_t *base_addr_is_alive(ir_node_t *node) {
    int i;
    int size = MAX_ALIVE_ADDR - alive_addr_empty;

    if (node->node_type != NODE_LVAL_NAME)
        die("INTERNAL_ERROR: expected NODE_LVAL_NAME");

    for (i=0; i<size; i++)
        if (alive_base_addr[i]->lval_name == node->lval_name)
            return alive_base_addr[i]->reg;

    return NULL;
}

void update_addr_alive(ir_node_t *node) {
    int i;
    int size = MAX_ALIVE_ADDR - alive_addr_empty;

    if (node->node_type != NODE_LVAL_NAME)
        die("INTERNAL_ERROR: expected NODE_LVAL_NAME");

    for (i=0; i<size; i++)
        if (alive_base_addr[i]->lval_name == node->lval_name) {
            alive_base_addr[i] = node;
            return;
        }
}

void mark_addr_alive(ir_node_t *node) {
    if (node->node_type != NODE_LVAL_NAME)
        die("INTERNAL_ERROR: expected NODE_LVAL_NAME");

    if (!alive_addr_empty)
        //ignore more alive addresses
        return;

    int idx = MAX_ALIVE_ADDR - alive_addr_empty;
    alive_addr_empty--;
    alive_base_addr[idx] = node;
}
#endif

reg_t *var_is_alive(char *name) {
    int i;
    int size = MAX_ALIVE_VARS - alive_vars_empty;

    for (i=0; i<size; i++)
        if (alive_vars[i].name == name)
            return alive_vars[i].reg;

    return NULL;
}

void update_alive_var(char *name, reg_t *reg) {
    int i;
    int size = MAX_ALIVE_VARS - alive_vars_empty;

    for (i=0; i<size; i++)
        if (alive_vars[i].name == name) {
            alive_vars[i].reg = reg;
            return;
        }
}

void mark_var_alive(char *name, reg_t *reg) {
    if (!alive_vars_empty)
        return;

    int idx = MAX_ALIVE_VARS - alive_vars_empty;
    alive_vars_empty--;
    alive_vars[idx].name = name;
    alive_vars[idx].reg = reg;
}

void new_final_tree() {
    final_tree[MAX_NUM_OF_MODULES - final_tree_empty] = final_tree_current;
    final_tree_empty--;
    final_tree_current = NULL;
}

void print_register(reg_t *reg) {
    switch (reg->type) {
    case REG_VIRT:
        printf("@%ld",reg->virtual);
        break;
    case REG_ALLOCATED:
        printf("%s",reg->physical->name);
        break;
    case REG_PHYSICAL:
        printf("%s",reg->name);
        break;
    }
}

void REG_ALLOCATE(reg_t *reg) {
    if (IS_REG_VIRT(reg)) {
        reg->physical = get_available_reg(reg->is);
        if (reg->physical) {
            reg->type = REG_ALLOCATED;
        }
    }
}

void register_allocate(instr_t *pc) {
    //if (IS_REG_VIRT(pc->Rd)) { pc->Rd->physical = get_available_reg(pc->Rd->is); }
    REG_ALLOCATE(pc->Rd);
    REG_ALLOCATE(pc->Rs);
    REG_ALLOCATE(pc->Rt);
}

void REG_FREE(reg_t *reg, unsigned long barrier) {
    if (reg && reg->type==REG_ALLOCATED && reg->die->id == barrier) { release_reg(reg->physical); }
}

void register_free(instr_t *pc) {
    REG_FREE(pc->Rd, pc->id);
    REG_FREE(pc->Rs, pc->id);
    REG_FREE(pc->Rt, pc->id);
}

void print_instr(instr_t *instr) {
    //printf("%ld: ",instr->id);

    if (instr->label) { printf("%-22s: ",instr->label); }
    else { printf("\t\t\t"); }

    printf("%-8s ",instr->mips_instr->name);

    switch (instr->mips_instr->fmt) {
    case FMT_RD_RS_RT:
        print_register(instr->Rd); COMMA();
        print_register(instr->Rs); COMMA();
        print_register(instr->Rt);
        break;
    case FMT_RD_RS:
        print_register(instr->Rd); COMMA();
        print_register(instr->Rs);
        break;
    case FMT_RS_RT:
        print_register(instr->Rs); COMMA();
        print_register(instr->Rt);
        break;
    case FMT_RD_RS_IMM_IMM:
        printf("__implement_me__");
        break;
    case FMT_RD:
        print_register(instr->Rd);
        break;
    case FMT_RD_IMM:
        print_register(instr->Rd); COMMA();
        printf("%d",instr->ival);
        break;
    case FMT_RD_IMM32:
        print_register(instr->Rd); COMMA();
        printf("%d",instr->ival);
        break;

#if (USE_PSEUDO_INSTR_LA==1)
    case FMT_RD_LABEL:
        print_register(instr->Rd); COMMA();
        printf("%s",instr->lval_name);
        break;
#endif

    case FMT_RS:
        print_register(instr->Rs);
        break;
    case FMT_RD_RS_SHIFT:
        print_register(instr->Rd); COMMA();
        print_register(instr->Rs); COMMA();
        printf("%d",instr->ival);
        break;
    case FMT_RD_RS_IMM:
        print_register(instr->Rd); COMMA();
        print_register(instr->Rs); COMMA();
        printf("%d",instr->ival);
        break;
    case FMT_LOAD:
        print_register(instr->Rd); COMMA();
        printf("%d",instr->ival);

        printf("(");
        print_register(instr->Rs);
        printf(")");
        break;
    case FMT_STORE:
        print_register(instr->Rs); COMMA();
        printf("%d",instr->ival);

        printf("(");
        print_register(instr->Rt);
        printf(")");
        break;
    case FMT_RS_RT_OFFSET:
        print_register(instr->Rs); COMMA();
        print_register(instr->Rt); COMMA();
        printf("%s",instr->goto_label);
        break;
    case FMT_RS_OFFSET:
        print_register(instr->Rs); COMMA();
        printf("%s",instr->goto_label);
        break;
    case FMT_ADDR:
        printf("%s",instr->goto_label);
        break;
    case FMT_OFFSET:
        printf("%s",instr->goto_label);
        break;
    case FMT_EMPTY:
        break;
    }

    /*
    if (instr->Rd && instr->Rd->live == instr)
        printf("\t\t\t@%ld -> lines : %ld..%ld",instr->Rd->virtual, instr->Rd->live->id, instr->Rd->die->id);

    if (instr->Rs && instr->Rs->live == instr)
        printf("\t\t\t@%ld -> lines : %ld..%ld",instr->Rs->virtual, instr->Rs->live->id, instr->Rs->die->id);

    if (instr->Rt && instr->Rt->live == instr)
        printf("\t\t\t@%ld -> lines : %ld..%ld",instr->Rt->virtual, instr->Rt->live->id, instr->Rt->die->id);
    */

    printf("\n");
}

void print_data_segment() {
    int i;
    int size;

    sem_t **pool;

    pool = main_program->symbol_table.pool;

    if (!pool)
        die("INTERNAL_ERROR: print_data_segment(): no pool");

    size = MAX_SYMBOLS - main_program->symbol_table.pool_empty;

    printf("\n");
    printf(".data\n");

    for (i=0; i<size; i++) {
        if (pool[i]->id_is==ID_VAR) {
            var_t *var = pool[i]->var;
            printf("%s: %s ",var->name,var->dot_data.is);

            if (!TYPE_IS_STANDARD(var->datatype) || var->dot_data.is==DOT_SPACE) {
                //print size
                printf("%d\n",var->dot_data.ival);
            } else {
                //print value
                switch (var->datatype->is) {
                case TYPE_INT:      printf("%d\t",var->dot_data.ival);    break;
                case TYPE_REAL:     printf("%f\t",var->dot_data.fval);    break;
                case TYPE_CHAR:     printf("'%c'\t",var->dot_data.cval);  break;
                case TYPE_BOOLEAN:  printf("%d\t",var->dot_data.cval);    break;
                }

                printf("\t#known value at compile time\n");
            }
        }
    }
}

void print_text_segment() {
    int i;
    instr_t *instr;

    printf("\n");
    printf(".text\n");
    printf(".global main\n");

    for(i=0;i<MAX_NUM_OF_MODULES;i++) {
        instr = final_tree[i];
        if (!instr) {
            return;
        }

        free_all_registers();
        while (instr) {
            register_allocate(instr);
            print_instr(instr);
            register_free(instr);
            instr = instr->next;
        }
        printf("\n");
    }
}

void print_assembly() {
    print_data_segment();
    print_text_segment();
}
