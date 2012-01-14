#include <stdio.h>
#include <stdlib.h>

#include "final_code.h"

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

instr_t *new_instruction(char *label,mips_instr_t *mips_instr) {
    instr_t *new_instr;

    new_instr = (instr_t*)malloc(sizeof(instr_t));

    new_instr->prev = NULL;
    new_instr->next = NULL;
    new_instr->last = new_instr;

    new_instr->label = label;
    new_instr->mips_instr = mips_instr;

    return new_instr;
}

instr_t *link_instructions(instr_t *child, instr_t *parent) {
    if (child && parent) {
        parent->last->next = child;
        child->prev = parent->last;
        parent->last = child->last;
        return parent;
    } else if (child) {
        return child;
    } else if (parent) {
        return parent;
    }
    return NULL;
}

void print_instr(instr_t *instr) {
    if (instr->label) { printf("%-22s: ",instr->label); }
    else { printf("\t\t\t"); }

    printf("%-8s ",instr->mips_instr->name);

    switch (instr->mips_instr->fmt) {
    case FMT_RD_RS_RT:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }

        if (instr->Rs) { printf("%s, ",instr->Rs->name);}
        else { printf("@%ld, ",instr->virt_Rs); }

        if (instr->Rt) { printf("%s",instr->Rt->name);}
        else { printf("@%ld",instr->virt_Rt); }
        break;
    case FMT_RD_RS:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }

        if (instr->Rs) { printf("%s",instr->Rs->name);}
        else { printf("@%ld",instr->virt_Rs); }
        break;
    case FMT_RS_RT:
        if (instr->Rs) { printf("%s, ",instr->Rs->name);}
        else { printf("@%ld, ",instr->virt_Rs); }

        if (instr->Rt) { printf("%s",instr->Rt->name);}
        else { printf("@%ld",instr->virt_Rt); }
        break;
    case FMT_RD_RS_IMM_IMM:
        //if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        //else { printf("@%ld, ",instr->virt_Rd); }

        //if (instr->Rs) { printf("%s, ",instr->Rs->name);}
        //else { printf("@%ld, ",instr->virt_Rs); }

        //printf("%d, ",instr->ival);
        printf("__implement_me__");
        break;
    case FMT_RD:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }
        break;
    case FMT_RD_IMM:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }

        printf("%d",instr->ival);
        break;
    case FMT_RD_IMM32:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }

        printf("%d",instr->ival);
        break;
    case FMT_RD_LABEL:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }
        break;
    case FMT_RS:
        if (instr->Rs) { printf("%s",instr->Rs->name);}
        else { printf("@%ld",instr->virt_Rs); }
        break;
    case FMT_RD_RS_SHIFT:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }

        if (instr->Rs) { printf("%s, ",instr->Rs->name);}
        else { printf("@%ld, ",instr->virt_Rs); }

        printf("%d",instr->ival);
        break;
    case FMT_RD_RS_IMM:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }

        if (instr->Rs) { printf("%s, ",instr->Rs->name);}
        else { printf("@%ld, ",instr->virt_Rs); }

        printf("%d",instr->ival);
        break;
    case FMT_LOAD:
        if (instr->Rd) { printf("%s, ",instr->Rd->name);}
        else { printf("@%ld, ",instr->virt_Rd); }

        printf("%d",instr->ival);

        if (instr->Rs) { printf("(%s)",instr->Rs->name);}
        else { printf("(@%ld)",instr->virt_Rs); }

        break;
    case FMT_STORE:
        if (instr->Rs) { printf("%s, ",instr->Rs->name);}
        else { printf("@%ld, ",instr->virt_Rs); }

        printf("%d",instr->ival);

        if (instr->Rt) { printf("(%s)",instr->Rt->name);}
        else { printf("(@%ld)",instr->virt_Rt); }
        break;
    case FMT_RS_RT_OFFSET:
        if (instr->Rs) { printf("%s, ",instr->Rs->name);}
        else { printf("@%ld, ",instr->virt_Rs); }

        if (instr->Rt) { printf("%s, ",instr->Rt->name);}
        else { printf("@%ld, ",instr->virt_Rt); }

        printf("%s",instr->goto_label);
        break;
    case FMT_RS_OFFSET:
        if (instr->Rs) { printf("%s, ",instr->Rs->name);}
        else { printf("@%ld, ",instr->virt_Rs); }

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
