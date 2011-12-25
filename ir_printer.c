#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "ir_printer.h"

#include "expr_toolbox.h"

void print_ir_node(ir_node_t *ir_node) {
    ir_node_t *tmp;

    if (ir_node->node_type!=NODE_DUMMY_LABEL) {
        //printf("R_%ld__",ir_node->R_register);
    }

    switch(ir_node->node_type) {
    case NODE_DUMMY_LABEL:
        //printf("%s: ",ir_node->label);
        return;
    case NODE_LOST_NODE:
        printf("%s ",ir_node->error);
        return;
    case NODE_BRANCH:
        print_ir_node(ir_node->ir_cond);
        printf("if $%ld goto: %s",ir_node->R_register,ir_node->jump_label);
        return;
    case NODE_JUMP_LINK:
        printf("jal %s",ir_node->jump_label);
        return;
    case NODE_JUMP:
        printf("j %s",ir_node->jump_label);
        return;
    case NODE_RETURN_SUBPROGRAM:
        printf("__return__");
        return;
    case NODE_INPUT_INT:
        printf("__read_int_to__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_INPUT_REAL:
        printf("__read_real_to__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_INPUT_BOOLEAN:
        printf("__read_boolean_to__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_INPUT_CHAR:
        printf("__read_char_to__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_INPUT_STRING:
        printf("__read_string_of_size__%d__to__",ir_node->ival);
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_OUTPUT_INT:
        printf("__print_int_from__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_OUTPUT_REAL:
        printf("__print_real_from__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_OUTPUT_BOOLEAN:
        printf("__print_boolean_from__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_OUTPUT_CHAR:
        printf("__print_char_from__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_OUTPUT_STRING:
        printf("__print_string_from__");
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_CONVERT_TO_INT:
        printf("\t\t\t\t");
        printf("__(integer)___");
        printf("(");
        print_ir_node(ir_node->ir_rval);
        printf(")");
        return;
    case NODE_CONVERT_TO_REAL:
        printf("\t\t\t\t");
        printf("__(real)___");
        printf("(");
        print_ir_node(ir_node->ir_rval);
        printf(")");
        return;
    case NODE_MEMCPY:
        printf("__memcpy__");
        print_ir_node(ir_node->address);
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_LOAD:
        printf("lw $%ld, ",ir_node->R_register);
        printf("{");
        print_ir_node(ir_node->address);
        printf("}");
        return;
    case NODE_BINARY_AND:
        printf("__&__");
        return;
    case NODE_BINARY_OR:
        printf("__|__");
        return;
    case NODE_BINARY_NOT:
        printf("__~__");
        return;
    case NODE_SHIFT_LEFT:
        printf("__shiftL__");
        return;
    case NODE_SHIFT_RIGHT:
        printf("__shiftR__");
        return;
    case NODE_LVAL:
        //printf("__lval__");
        printf("[");
        print_ir_node(ir_node->offset);
        printf("]");
        printf("{");
        print_ir_node(ir_node->address);
        printf("}");
        return;
    case NODE_HARDCODED_LVAL:
        //printf("{");
        print_ir_node(ir_node->address);
        //printf("}");
        return;
    case NODE_RVAL:
        if (ir_node->op_rval!=OP_NOT && ir_node->op_rval!=OP_SIGN) {
            switch (ir_node->ir_rval->node_type) {
            case NODE_ASSIGN:
            case NODE_JUMP_LINK:
                //case NODE_LOAD:
                //first node of prepare_stack
                tmp = ir_node->ir_rval;
                while (tmp) {
                    //if (tmp->node_type==NODE_LOAD) { break; }
                    print_ir_node(tmp);
                    tmp = tmp->next;
                    printf("\n\t\t\t");
                }
                break;
            case NODE_HARDCODED_RVAL:
                printf("addi $%ld, $0, %0.2f",ir_node->ir_rval->R_register,ir_node->ir_rval->fval);
                printf("\n\t\t\t");
                break;
            default:
                print_ir_node(ir_node->ir_rval);
                printf("\n\t\t\t");
                break;
            }
        }

        switch (ir_node->ir_rval2->node_type) {
        case NODE_ASSIGN:
        case NODE_JUMP_LINK:
            //case NODE_LOAD:
            //first node of prepare_stack
            tmp = ir_node->ir_rval2;
            while (tmp) {
                //if (tmp->node_type==NODE_LOAD) { break; }
                print_ir_node(tmp);
                tmp = tmp->next;
                printf("\n\t\t\t");
            }
            break;
        case NODE_HARDCODED_RVAL:
            printf("addi $%ld, $0, %0.2f",ir_node->ir_rval2->R_register,ir_node->ir_rval2->fval);
            printf("\n\t\t\t");
            break;
        default:
            print_ir_node(ir_node->ir_rval2);
            printf("\n\t\t\t");
            break;
        }

        if (ir_node->op_rval!=OP_NOT && ir_node->op_rval!=OP_SIGN) {
            printf("%s_ $%ld, $%ld, $%ld",op_literal(ir_node->op_rval),
                   ir_node->R_register,
                   ir_node->ir_rval->last->R_register,
                   ir_node->ir_rval2->last->R_register);
            //printf("_%d_",ir_node->ir_rval->node_type);
        } else {
            printf("%s_ $%ld, $%ld",op_literal(ir_node->op_rval),
                   ir_node->R_register,
                   ir_node->ir_rval2->last->R_register);
        }

        return;
    case NODE_HARDCODED_RVAL:
        //printf("addi $%ld, $0, %0.2f",ir_node->R_register,ir_node->fval);
        //printf("%0.2f",ir_node->fval);
        //printf("__%ld__",ir_node->R_register);
        printf("%d",ir_node->ival);
        return;
    case NODE_INIT_NULL_SET:
        printf("INIT_NULL_SET");
        return;
    case NODE_ADD_ELEM_TO_SET:
        printf("add_elem_to_set");
        return;
    case NODE_ADD_ELEM_RANGE_TO_SET:
        printf("add_elem_range_to_set");
        return;
    case NODE_CHECK_INOP_BITMAPPED:
        printf("check_inop_bitmapped");
        return;
    case NODE_ASSIGN:
        //tmp = ir_node->ir_rval;
        switch (ir_node->ir_rval->node_type) {
        case NODE_ASSIGN:
        case NODE_JUMP_LINK:
            //case NODE_LOAD:
            //first node of prepare_stack
            tmp = ir_node->ir_rval;
            while (tmp) {
                if (tmp->node_type==NODE_LOAD) { break; }
                print_ir_node(tmp);
                    tmp = tmp->next;
                    printf("\n\t\t\t");
            }
            break;
        default:
            break;
        }

        if (ir_node->ir_rval->last->node_type==NODE_HARDCODED_RVAL) {
            printf("addi $%ld, $0, %d",ir_node->ir_rval->last->R_register,ir_node->ir_rval->last->ival);
        } else {
            print_ir_node(ir_node->ir_rval->last);
        }
        printf("\n\t\t\t");

        printf("sw $%ld",ir_node->ir_rval->last->R_register);
        //print_ir_node(ir_node->ir_rval->last);
        printf(", {");
        print_ir_node(ir_node->address);
        printf("}");
        return;
    case NODE_ASSIGN_SET:
        printf("assign_set \t");
        print_ir_node(ir_node->address);
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_ASSIGN_STRING:
        printf("assign_string \t");
        print_ir_node(ir_node->address);
        print_ir_node(ir_node->ir_lval);
        return;
    default:
        printf("INTERNAL_ERROR: UNKNOWN NODE TYPE\n");
        exit(EXIT_FAILURE);
    }

    //if (ir_node->next) {
    //    printf("\n\t\t\t");
    //    print_ir_node(ir_node->next);
    //}
}

void print_all_modules() {
    int i;
    ir_node_t *module;

    for(i=0;i<MAX_NUM_OF_MODULES;i++) {
        module = ir_root_module[i];
        print_module(module);
    }
}

void print_module(ir_node_t *module) {
    ir_node_t *ir_node;

    if (!module) {
        return;
    }

    ir_node = module;

    //the very first label is the module name
    //print_ir_node(ir_node);
    //printf("%-22s: ",ir_node->label);
    //ir_node = ir_node->next;

    while(ir_node) {
        //every node can have a label
        if (ir_node->label) {
            printf("%-22s: ",ir_node->label);
        } else {
            printf("\t\t\t");
        }

        if (ir_node->node_type!=NODE_DUMMY_LABEL) {
            print_ir_node(ir_node);
            printf("\n");
        } else {
            print_ir_node(ir_node);
            ir_node = ir_node->next;
            if (ir_node) {
                print_ir_node(ir_node);
                printf("\n");
            } else {
                //we hit NULL, end of module
                return;
            }
        }

        ir_node = ir_node->next;
    }
}
