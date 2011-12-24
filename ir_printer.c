#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "ir_printer.h"

#include "expr_toolbox.h"

void print_ir_node(ir_node_t *ir_node) {
    if (ir_node->node_type!=NODE_DUMMY_LABEL) {
        //printf("R_%ld \t",ir_node->R_register);
    }

    switch(ir_node->node_type) {
    case NODE_DUMMY_LABEL:
        //printf("%s: ",ir_node->label);
        break;
    case NODE_LOST_NODE:
        printf("%s ",ir_node->error);
        break;
    case NODE_BRANCH:
        printf("if");
        print_ir_node(ir_node->ir_cond);
        printf(" goto: %s",ir_node->jump_label);
        break;
    case NODE_JUMP_LINK:
        printf("jump_link %s",ir_node->jump_label);
        break;
    case NODE_JUMP:
        printf("jump %s",ir_node->jump_label);
        break;
    case NODE_RETURN_SUBPROGRAM:
        printf("__return__");
        break;
    case NODE_INPUT_INT:
        printf("__read_int_to__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_INPUT_REAL:
        printf("__read_real_to__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_INPUT_BOOLEAN:
        printf("__read_boolean_to__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_INPUT_CHAR:
        printf("__read_char_to__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_INPUT_STRING:
        printf("__read_string_of_size__%d__to__",ir_node->ival);
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_OUTPUT_INT:
        printf("__print_int_from__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_OUTPUT_REAL:
        printf("__print_real_from__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_OUTPUT_BOOLEAN:
        printf("__print_boolean_from__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_OUTPUT_CHAR:
        printf("__print_char_from__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_OUTPUT_STRING:
        printf("__print_string_from__");
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_CONVERT_TO_INT:
        printf("\t\t\t\t");
        printf("__(integer)___");
        printf("(");
        print_ir_node(ir_node->ir_rval);
        printf(")");
        break;
    case NODE_CONVERT_TO_REAL:
        printf("\t\t\t\t");
        printf("__(real)___");
        printf("(");
        print_ir_node(ir_node->ir_rval);
        printf(")");
        break;
    case NODE_MEMCPY:
        printf("__memcpy__");
        print_ir_node(ir_node->address);
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_LOAD:
        printf("mem{");
        print_ir_node(ir_node->address);
        printf("}");
        break;
    case NODE_BINARY_AND:
        printf("__&__");
        break;
    case NODE_BINARY_OR:
        printf("__|__");
        break;
    case NODE_BINARY_NOT:
        printf("__~__");
        break;
    case NODE_SHIFT_LEFT:
        printf("__shiftL__");
        break;
    case NODE_SHIFT_RIGHT:
        printf("__shiftR__");
        break;
    case NODE_LVAL:
        //printf("__lval__");
        printf("[");
        print_ir_node(ir_node->offset);
        printf("]mem");
        printf("{");
        print_ir_node(ir_node->address);
        printf("}");
        break;
    case NODE_HARDCODED_LVAL:
        //address and offset are together, see "calculate_lvalue()" in ir_toolbox.c
        //printf("{");
        print_ir_node(ir_node->address);
        //printf("}");
        break;
    case NODE_RVAL:
        printf("(");
        if (ir_node->op_rval!=OP_NOT && ir_node->op_rval!=OP_SIGN) {
            print_ir_node(ir_node->ir_rval);
        }
        printf(" %s ",op_literal(ir_node->op_rval));
        print_ir_node(ir_node->ir_rval2);
        printf(")");
        break;
    case NODE_HARDCODED_RVAL:
        printf("%0.2f",ir_node->fval);
        break;
    case NODE_INIT_NULL_SET:
        printf("INIT_NULL_SET");
        break;
    case NODE_ADD_ELEM_TO_SET:
        printf("add_elem_to_set");
        break;
    case NODE_ADD_ELEM_RANGE_TO_SET:
        printf("add_elem_range_to_set");
        break;
    case NODE_CHECK_INOP_BITMAPPED:
        printf("check_inop_bitmapped");
        break;
    case NODE_ASSIGN:
        printf("@");
        print_ir_node(ir_node->address);
        printf(":=");
        print_ir_node(ir_node->ir_rval);
        printf("@");
        break;
    case NODE_ASSIGN_SET:
        printf("assign_set \t");
        print_ir_node(ir_node->address);
        print_ir_node(ir_node->ir_lval);
        break;
    case NODE_ASSIGN_STRING:
        printf("assign_string \t");
        print_ir_node(ir_node->address);
        print_ir_node(ir_node->ir_lval);
        break;
    default:
        printf("INTERNAL_ERROR: UNKNOWN NODE TYPE\n");
        exit(EXIT_FAILURE);
    }
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
