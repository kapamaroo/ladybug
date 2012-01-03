#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "ir_printer.h"
#include "err_buff.h"

char *op_to_instruction(op_t op);

void print_ir_node(ir_node_t *ir_node) {
    ir_node_t *tmp;

    if (ir_node->label) {
        //printf("__%s__",ir_node->label);
    }

    if (ir_node->node_type!=NODE_DUMMY_LABEL) {
        //printf("R_%ld__",ir_node->virt_reg);
    }

    switch(ir_node->node_type) {
    case NODE_DUMMY_LABEL:
        //printf("%s: ",ir_node->label);
        return;
    case NODE_BRANCH:
        switch (ir_node->op_rval) {
        case OP_OR:
        case OP_AND:
            print_ir_node(ir_node->ir_rval);
            printf("\n\t\t\t");
            print_ir_node(ir_node->ir_rval2);
            break;
        case OP_NOT:
            die("UNEXPECTED ERROR: ir_printer: NODE_BRANCH_COND: not operator still alive??");
            printf("__%s__\n\t\t\t",op_to_instruction(ir_node->op_rval));
            break;
        default:
            //die("UNEXPENTED_ERROR: print branch ir");
            //print_ir_node();
            break;
        }
        return;
    case NODE_JUMP_LINK:
        printf("jal %s",ir_node->ir_goto->label);
        return;
    case NODE_JUMP:
        printf("j %s",ir_node->ir_goto->label);
        return;
    case NODE_RETURN_SUBPROGRAM:
        printf("jr $ra");
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
        print_ir_node(ir_node->ir_rval);
        printf("\n\t\t\t");
        printf("_convert_to_int $%ld $%ld",ir_node->virt_reg,ir_node->ir_rval->last->virt_reg);
        return;
    case NODE_CONVERT_TO_REAL:
        print_ir_node(ir_node->ir_rval);
        printf("\n\t\t\t");
        printf("_convert_to_real $%ld $%ld",ir_node->virt_reg,ir_node->ir_rval->last->virt_reg);
        return;
    case NODE_MEMCPY:
        printf("__memcpy__");
        print_ir_node(ir_node->address);
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_LOAD:
        printf("lw $%ld, ",ir_node->virt_reg);
        printf("{");
        print_ir_node(ir_node->address);
        printf("}");
        return;
        //case NODE_BINARY_AND:
        //printf("__&__");
        //return;
        //case NODE_BINARY_OR:
        //printf("__|__");
        //return;
        //case NODE_BINARY_NOT:
        //printf("__~__");
        //return;
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
        if (ir_node->label) {
            printf("__%s__",ir_node->label);
        }

        if (ir_node->op_rval!=OP_NOT && ir_node->op_rval!=OP_SIGN) {
            //first node of prepare_stack
            switch (ir_node->ir_rval->node_type) {
            case NODE_HARDCODED_RVAL:
                printf("addi $%ld, $0, %0.2f",ir_node->ir_rval->virt_reg,ir_node->ir_rval->fval);
                printf("\n\t\t\t");
                break;
            case NODE_RVAL:
                print_ir_node(ir_node->ir_rval);
                printf("\n\t\t\t");
                break;
            case NODE_RVAL_ARCH:
                break;
            case NODE_ASSIGN:
            case NODE_JUMP_LINK:
            case NODE_LOAD:
            default:
                tmp = ir_node->ir_rval;
                while (tmp) {
                    //if (tmp->node_type==NODE_LOAD) { break; }
                    print_ir_node(tmp);
                    printf("\n\t\t\t");
                    tmp = tmp->next;
                }
                break;
            }
        }

        switch (ir_node->ir_rval2->node_type) {
        case NODE_HARDCODED_RVAL:
            printf("addi $%ld, $0, %0.2f",ir_node->ir_rval2->virt_reg,ir_node->ir_rval2->fval);
            printf("\n\t\t\t");
            break;
        case NODE_RVAL:
            print_ir_node(ir_node->ir_rval2);
            printf("\n\t\t\t");
            break;
        case NODE_RVAL_ARCH:
            break;
        case NODE_ASSIGN:
        case NODE_JUMP_LINK:
        case NODE_LOAD:
        default:
            tmp = ir_node->ir_rval2;
            while (tmp) {
                //if (tmp->node_type==NODE_LOAD) { break; }
                print_ir_node(tmp);
                printf("\n\t\t\t");
                tmp = tmp->next;
            }
            break;
        }

        printf("%s ",op_to_instruction(ir_node->op_rval));

        //print my reg
        switch (ir_node->op_rval) {
        case RELOP_B:	// '>'
        case RELOP_BE:	// '>='
        case RELOP_L:	// '<'
        case RELOP_LE:	// '<='
        case RELOP_NE:	// '<>'
        case RELOP_EQU:	// '='
        case RELOP_IN:	// 'in'
        case OP_MULT:   // '*'
        case OP_DIV:    // 'div'
            break;
        default:
            printf("$%ld, ",ir_node->virt_reg);
            break;
        }

        if (ir_node->op_rval!=OP_NOT && ir_node->op_rval!=OP_SIGN) {
            if (ir_node->ir_rval->last->node_type==NODE_RVAL_ARCH) {
                print_ir_node(ir_node->ir_rval->last);
                printf(", ");
            } else {
                printf("$%ld, ",ir_node->ir_rval->last->virt_reg);
            }
        }

        if (ir_node->ir_rval2->last->node_type==NODE_RVAL_ARCH) {
            print_ir_node(ir_node->ir_rval2->last);
        } else {
            printf("$%ld",ir_node->ir_rval2->last->virt_reg);
        }

        //print label of branches
        switch (ir_node->op_rval) {
        case RELOP_B:	// '>'
        case RELOP_BE:	// '>='
        case RELOP_L:	// '<'
        case RELOP_LE:	// '<='
        case RELOP_NE:	// '<>'
        case RELOP_EQU:	// '='
        case RELOP_IN:	// 'in'
        case OP_AND:    // 'and'
        case OP_OR:     // 'or'
            printf(" goto: %s",ir_node->ir_goto->label);
            //printf("\n\t\t\t");
            break;
        case OP_NOT:    // 'not'
            die("UNEXPECTED ERROR: ir_printer: NODE_RVAL: not operator still alive??");
        default:
            break;
        }

        if (ir_node->next) {
            printf("\n\t\t\t");
            print_ir_node(ir_node->next);
        }
        return;
    case NODE_RVAL_ARCH:
        printf("%s",ir_node->reg->name);
        return;
    case NODE_HARDCODED_RVAL:
        //printf("addi $%ld, $0, %0.2f",ir_node->virt_reg,ir_node->fval);
        //printf("%0.2f",ir_node->fval);
        //printf("__%ld__",ir_node->virt_reg);
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
        switch (ir_node->ir_rval->node_type) {
        case NODE_HARDCODED_RVAL:
            printf("addi $%ld, $0, %0.2f",ir_node->ir_rval->virt_reg,ir_node->ir_rval->fval);
            printf("\n\t\t\t");
            break;
        case NODE_RVAL:
            print_ir_node(ir_node->ir_rval);
            printf("\n\t\t\t");
            break;
        case NODE_RVAL_ARCH:
            break;
        case NODE_ASSIGN:
        case NODE_JUMP_LINK:
        case NODE_LOAD:
        default:
            tmp = ir_node->ir_rval;
            while (tmp) {
                //if (tmp->node_type==NODE_LOAD) { break; }
                print_ir_node(tmp);
                printf("\n\t\t\t");
                tmp = tmp->next;
            }
            break;
        }

        printf("sw ");

        if (ir_node->ir_rval->last->node_type==NODE_RVAL_ARCH) {
            print_ir_node(ir_node->ir_rval->last);
            //printf(", ");
        } else {
            printf("$%ld",ir_node->ir_rval->last->virt_reg);
        }

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
        //default:
        //die("INTERNAL_ERROR: UNKNOWN NODE TYPE");
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
        module = ir_root_tree[i];
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

char *op_to_instruction(op_t op) {
    switch (op) {
    case RELOP_B:	// '>'
        return "bgt";
    case RELOP_BE:	// '>='
        return "bge";
    case RELOP_L:	// '<'
    	return "blt";
    case RELOP_LE:	// '<='
    	return "ble";
    case RELOP_NE:	// '<>'
    	return "bne";
    case RELOP_EQU:	// '='
        return "beq";
    case RELOP_IN:	// 'in'
    	return "in";
    case OP_PLUS:	// '+'
    	return "add";
    case OP_MINUS:	// '-'
    	return "sub";
    case OP_MULT:	// '*'
    	return "mult";
    case OP_RDIV:	// '/'
    	return "fp.div";
    case OP_DIV:       	// 'div'
    	return "div";
    case OP_MOD:       	// 'mod'
    	return "div";
    case OP_AND:       	// 'and'
    	return "and";
    case OP_OR:		// 'or'
    	return "or";
    case OP_NOT:       	// 'not'
        return "not"; //$reg xor 11111111111
    case OP_IGNORE:
        //return "__op_IGNORE__";
    case OP_SIGN: 	//dummy operator, to determine when the the OP_PLUS, OP_MINUS are used as sign
    	//return "op_SIGN";
    default:
        die("UNEXPECTED_ERROR: 04");
        return NULL; //keep the compiler happy
    }
}
