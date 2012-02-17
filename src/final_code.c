#include <stdio.h>
#include <stdlib.h>

#include "final_code.h"
#include "instruction_set.h"
#include "reg.h"

#define COMMA() printf(", ")

instr_t *final_tree[MAX_NUM_OF_MODULES];
int final_tree_empty;

instr_t *final_tree_current;

void init_final_code() {
    int i;

    final_tree_empty = MAX_NUM_OF_MODULES;

    for(i=0;i<MAX_NUM_OF_MODULES;i++) {
        final_tree[i] = NULL;
    }

    final_tree_current = NULL;
}

void new_final_tree() {
    final_tree[MAX_NUM_OF_MODULES - final_tree_empty] = final_tree_current;
    final_tree_empty--;
    final_tree_current = NULL;
}

void print_register(reg_t *reg) {
    if (reg->is != REG_VIRT) { printf("%s",reg->name);}
    else { printf("@%ld",reg->virtual); }
}

void print_instr(instr_t *instr) {
    printf("%ld: ",instr->id);

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
    case FMT_RD_LABEL:
        print_register(instr->Rd);
        break;
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


    if (IS_REG_VIRT(instr->Rd) &&
        instr->Rd->live == instr)
        printf("\t\t\t@%ld -> lines : %ld..%ld",instr->Rd->virtual, instr->Rd->live->id, instr->Rd->die->id);

    if (IS_REG_VIRT(instr->Rs) &&
        instr->Rs->live == instr)
        printf("\t\t\t@%ld -> lines : %ld..%ld",instr->Rs->virtual, instr->Rs->live->id, instr->Rs->die->id);

    if (IS_REG_VIRT(instr->Rt) &&
        instr->Rt->live == instr)
        printf("\t\t\t@%ld -> lines : %ld..%ld",instr->Rt->virtual, instr->Rt->live->id, instr->Rt->die->id);

}

void print_assembly() {
    int i;
    instr_t *instr;

    for(i=0;i<MAX_NUM_OF_MODULES;i++) {
        if (!final_tree[i]) {
            return;
        }

        instr = final_tree[i];
        while (instr) {
            print_instr(instr);
            printf("\n");
            instr = instr->next;
        }
        printf("\n");
    }
}
