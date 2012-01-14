#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "ir_printer.h"
#include "final_code.h"
#include "err_buff.h"

char *op_to_instruction_literal(op_t op);
mips_instr_t *op_to_mips_instr_t(op_t op, type_t datatype);

void print_ir_node(ir_node_t *ir_node) {
    ir_node_t *tmp;
    instr_t *new_instr;

    if (ir_node->label) {
        printf("%-22s: ",ir_node->label);
    }

    switch(ir_node->node_type) {
    case NODE_DUMMY_LABEL:
        new_instr = new_instruction(ir_node->label,&I_nop);
        final_tree_current = link_instructions(new_instr,final_tree_current);
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
        default:
            die("UNEXPENTED_ERROR: print branch ir");
            break;
        }
        return;
    case NODE_JUMP_LINK:
        new_instr = new_instruction(ir_node->label,&I_jal);
        new_instr->goto_label = ir_node->ir_goto->label;
        final_tree_current = link_instructions(new_instr,final_tree_current);
        printf("jal %s",ir_node->ir_goto->label);
        return;
    case NODE_JUMP:
        new_instr = new_instruction(ir_node->label,&I_j);
        new_instr->goto_label = ir_node->ir_goto->label;
        final_tree_current = link_instructions(new_instr,final_tree_current);
        printf("j %s",ir_node->ir_goto->label);
        return;
    case NODE_RETURN_SUBPROGRAM:
        new_instr = new_instruction(ir_node->label,&I_jr);
        new_instr->Rs = &R_ra;
        final_tree_current = link_instructions(new_instr,final_tree_current);
        printf("jr $ra");
        return;
    case NODE_SYSCALL:
        //the first instruction sets the label
        if (ir_node->label) {
            new_instr = new_instruction(ir_node->label,&I_nop);
            final_tree_current = link_instructions(new_instr,final_tree_current);
        }

        //prepare
        switch (ir_node->syscall_num) {
        case SVC_PRINT_INT:
        case SVC_PRINT_CHAR:
        case SVC_PRINT_STRING:
            //set the register first
            ir_node->ir_rval->reg = &R_a0;
            print_ir_node(ir_node->ir_rval);
            break;
        case SVC_PRINT_REAL:
            //set the register first
            ir_node->ir_rval->reg = arch_fp[12];
            print_ir_node(ir_node->ir_rval);
            break;
        case SVC_READ_INT:
        case SVC_READ_CHAR:
        case SVC_READ_REAL:
            //it is safer to parse the address first, because it may change the special
            //registers before we use them
            print_ir_node(ir_node->address);
            break;
        case SVC_READ_STRING:
            //pass address of string
            new_instr = new_instruction(NULL,&I_move);
            new_instr->Rd = &R_a0;
            new_instr->virt_Rs = ir_node->ir_rval->virt_reg;
            final_tree_current = link_instructions(new_instr,final_tree_current);

            //pass size of string
            new_instr = new_instruction(NULL,&I_li);
            new_instr->Rd = &R_a1;
            new_instr->ival = ir_node->ival;
            final_tree_current = link_instructions(new_instr,final_tree_current);
            break;
        case SVC_EXIT:
            break;
        }

        //set service number
        new_instr = new_instruction(NULL,&I_li);
        new_instr->Rd = &R_v0;
        new_instr->ival = ir_node->syscall_num;
        final_tree_current = link_instructions(new_instr,final_tree_current);

        //do syscall
        new_instr = new_instruction(NULL,&I_syscall);
        final_tree_current = link_instructions(new_instr,final_tree_current);

        //finally
        switch (ir_node->syscall_num) {
        case SVC_PRINT_INT:
        case SVC_PRINT_CHAR:
        case SVC_PRINT_STRING:
        case SVC_PRINT_REAL:
        case SVC_READ_STRING:
        case SVC_EXIT:
            break;
        case SVC_READ_INT:
        case SVC_READ_CHAR:
            //store input to register
            new_instr = new_instruction(NULL,&I_sw);
            new_instr->Rt = ir_node->address->last->reg;
            new_instr->virt_Rt = ir_node->address->last->virt_reg;
            new_instr->ival = ir_node->offset->ival;
            new_instr->Rs = &R_a0;
            final_tree_current = link_instructions(new_instr,final_tree_current);
            break;
        case SVC_READ_REAL:
            //save input to mem
            new_instr = new_instruction(NULL,&I_swc1);
            new_instr->Rt = ir_node->address->last->reg;
            new_instr->virt_Rt = ir_node->address->last->virt_reg;
            new_instr->ival = ir_node->offset->ival;
            new_instr->Rs = arch_fp[0];
            final_tree_current = link_instructions(new_instr,final_tree_current);
            break;
        }
        break;
    case NODE_CONVERT_TO_INT:
        if (ir_node->label) {
            new_instr = new_instruction(ir_node->label,&I_nop);
            final_tree_current = link_instructions(new_instr,final_tree_current);
        }

        print_ir_node(ir_node->ir_rval);

        new_instr = new_instruction(NULL,&I_cvt_w_s);
        new_instr->virt_Rd = ir_node->virt_reg;
        new_instr->virt_Rs = ir_node->ir_rval->last->virt_reg;
        final_tree_current = link_instructions(new_instr,final_tree_current);

        printf("\n\t\t\t");
        printf("%s $%ld $%ld",I_cvt_w_s.name,ir_node->virt_reg,ir_node->ir_rval->last->virt_reg);
        return;
    case NODE_CONVERT_TO_REAL:
        if (ir_node->label) {
            new_instr = new_instruction(ir_node->label,&I_nop);
            final_tree_current = link_instructions(new_instr,final_tree_current);
        }

        print_ir_node(ir_node->ir_rval);

        new_instr = new_instruction(NULL,&I_cvt_s_w);
        new_instr->virt_Rd = ir_node->virt_reg;
        new_instr->virt_Rs = ir_node->ir_rval->last->virt_reg;
        final_tree_current = link_instructions(new_instr,final_tree_current);

        printf("\n\t\t\t");
        printf("%s $%ld $%ld",I_cvt_s_w.name,ir_node->virt_reg,ir_node->ir_rval->last->virt_reg);
        return;
    case NODE_MEMCPY:
        printf("__memcpy__");
        print_ir_node(ir_node->address);
        print_ir_node(ir_node->ir_lval);
        return;
    case NODE_LOAD:
        if (ir_node->label) {
            new_instr = new_instruction(ir_node->label,&I_nop);
            final_tree_current = link_instructions(new_instr,final_tree_current);
        }

        printf("lw $%ld, ",ir_node->virt_reg);
        print_ir_node(ir_node->ir_lval);

        if (ir_node->data_is==TYPE_REAL) {
            //load directly to c1
            new_instr = new_instruction(NULL,&I_lwc1);
        } else {
            new_instr = new_instruction(NULL,&I_lw);
        }

        new_instr->virt_Rd = ir_node->virt_reg;

        new_instr->Rs = ir_node->ir_lval->address->last->reg;
        new_instr->virt_Rs = ir_node->ir_lval->address->last->virt_reg;
        new_instr->ival = ir_node->ir_lval->offset->ival;

        final_tree_current = link_instructions(new_instr,final_tree_current);
        return;
    case NODE_SHIFT_LEFT:
        printf("__shiftL__");
        return;
    case NODE_SHIFT_RIGHT:
        printf("__shiftR__");
        return;
    case NODE_LVAL:
        print_ir_node(ir_node->offset);
        printf("(");
        print_ir_node(ir_node->address);
        printf(")");
        return;
    case NODE_RVAL:
        if (ir_node->label) {
            new_instr = new_instruction(ir_node->label,&I_nop);
            final_tree_current = link_instructions(new_instr,final_tree_current);
        }

        if (ir_node->op_rval!=OP_NOT && ir_node->op_rval!=OP_SIGN) {
            //first node of prepare_stack
            switch (ir_node->ir_rval->node_type) {
            case NODE_HARDCODED_RVAL:
                new_instr = new_instruction(ir_node->ir_rval->label,&I_addi);
                new_instr->virt_Rd = ir_node->ir_rval->virt_reg;
                new_instr->Rs = &R_zero;
                new_instr->ival = ir_node->ir_rval->ival;
                final_tree_current = link_instructions(new_instr,final_tree_current);

                printf("addi $%ld, $0, %0.2f",ir_node->ir_rval->virt_reg,ir_node->ir_rval->fval);
                printf("\n\t\t\t");
                break;
            case NODE_RVAL:
                print_ir_node(ir_node->ir_rval);
                printf("\n\t\t\t");
                break;
            case NODE_RVAL_ARCH:
                break;
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
            new_instr = new_instruction(ir_node->ir_rval2->label,&I_addi);
            new_instr->virt_Rd = ir_node->ir_rval2->virt_reg;
            new_instr->Rs = &R_zero;
            new_instr->ival = ir_node->ir_rval2->ival;
            final_tree_current = link_instructions(new_instr,final_tree_current);

            printf("addi $%ld, $0, %0.2f",ir_node->ir_rval2->virt_reg,ir_node->ir_rval2->fval);
            printf("\n\t\t\t");
            break;
        case NODE_RVAL:
            print_ir_node(ir_node->ir_rval2);
            printf("\n\t\t\t");
            break;
        case NODE_RVAL_ARCH:
            break;
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

        printf("%s ",op_to_instruction_literal(ir_node->op_rval));

        //print my reg
        switch (ir_node->op_rval) {
        case RELOP_B:	// '>'
        case RELOP_BE:	// '>='
        case RELOP_L:	// '<'
        case RELOP_LE:	// '<='
        case RELOP_NE:	// '<>'
        case RELOP_EQU:	// '='
        case RELOP_IN:	// 'in'
            break;
        case OP_MULT:   // '*'
        case OP_DIV:    // 'div'
            //break;
        default:
            printf("$%ld, ",ir_node->virt_reg);
            break;
        }

        new_instr = new_instruction(NULL,op_to_mips_instr_t(ir_node->op_rval,ir_node->data_is));

        //pass the Rd, may be needed by optimizers
        new_instr->virt_Rd = ir_node->virt_reg;
        new_instr->Rd = ir_node->reg;

        if (ir_node->op_rval!=OP_NOT && ir_node->op_rval!=OP_SIGN) {
            if (ir_node->ir_rval->last->node_type==NODE_RVAL_ARCH) {
                new_instr->Rs = ir_node->ir_rval->last->reg;
                print_ir_node(ir_node->ir_rval->last);
                printf(", ");
            } else {
                new_instr->virt_Rs = ir_node->ir_rval->last->virt_reg;
                printf("$%ld, ",ir_node->ir_rval->last->virt_reg);
            }
        }

        if (ir_node->ir_rval2->last->node_type==NODE_RVAL_ARCH) {
            new_instr->Rt = ir_node->ir_rval2->last->reg;
            print_ir_node(ir_node->ir_rval2->last);
        } else {
            new_instr->virt_Rt = ir_node->ir_rval2->last->virt_reg;
            printf("$%ld",ir_node->ir_rval2->last->virt_reg);
        }

        //print label of branches
        switch (ir_node->op_rval) {
        case RELOP_L:	// '<'
        case RELOP_LE:	// '<='
        case RELOP_EQU:	// '='
            if (ir_node->data_is==TYPE_REAL) {
                final_tree_current = link_instructions(new_instr,final_tree_current);
                new_instr = new_instruction(NULL,&I_bc1t);
            }

            new_instr->goto_label = ir_node->ir_goto->label;

            printf(" goto: %s",ir_node->ir_goto->label);
            break;
        case RELOP_NE:	// '<>'
        case RELOP_B:	// '>'
        case RELOP_BE:	// '>='
            if (ir_node->data_is==TYPE_REAL) {
                final_tree_current = link_instructions(new_instr,final_tree_current);
                new_instr = new_instruction(NULL,&I_bc1f);
            }

            new_instr->goto_label = ir_node->ir_goto->label;

            printf(" goto: %s",ir_node->ir_goto->label);
            break;
        case OP_AND:    // 'and'
        case OP_OR:     // 'or'
        case OP_NOT:    // 'not'
            die("UNEXPECTED ERROR: ir_printer: NODE_RVAL: logical and/or operator still alive??");
        case RELOP_IN:	// 'in'
            die("UNEXPECTED ERROR: ir_printer: NODE_RVAL: in operator still alive??");
        default:
            break;
        }

        final_tree_current = link_instructions(new_instr,final_tree_current);

        if (ir_node->next) {
            printf("\n\t\t\t");
            print_ir_node(ir_node->next);
        }
        return;
    case NODE_RVAL_ARCH:
        printf("%s",ir_node->reg->name);
        return;
    case NODE_HARDCODED_RVAL:
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
        if (ir_node->label) {
            new_instr = new_instruction(ir_node->label,&I_nop);
            final_tree_current = link_instructions(new_instr,final_tree_current);
        }

        switch (ir_node->ir_rval->node_type) {
        case NODE_HARDCODED_RVAL:
            new_instr = new_instruction(ir_node->ir_rval->label,&I_addi);
            new_instr->virt_Rd = ir_node->ir_rval->virt_reg;
            new_instr->Rs = &R_zero;
            new_instr->ival = ir_node->ir_rval->ival;
            final_tree_current = link_instructions(new_instr,final_tree_current);

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

        //parse the address first
        print_ir_node(ir_node->ir_lval);

        printf("sw ");

        if (ir_node->data_is==TYPE_REAL) {
            //store directly from c1
            new_instr = new_instruction(NULL,&I_swc1);
        } else {
            new_instr = new_instruction(NULL,&I_sw);
        }

        if (ir_node->ir_rval->last->node_type==NODE_RVAL_ARCH) {
            new_instr->Rs = ir_node->ir_rval->last->reg;
            print_ir_node(ir_node->ir_rval->last);
            printf(", ");
        } else {
            new_instr->virt_Rs = ir_node->ir_rval->last->virt_reg;
            printf("$%ld, ",ir_node->ir_rval->last->virt_reg);
        }

        new_instr->Rt = ir_node->ir_lval->address->last->reg;
        new_instr->virt_Rt = ir_node->ir_lval->address->last->virt_reg;
        new_instr->ival = ir_node->ir_lval->offset->ival;

        final_tree_current = link_instructions(new_instr,final_tree_current);
        return;
    case NODE_ASSIGN_SET:
        printf("assign_set \t");
        print_ir_node(ir_node->ir_lval);
        print_ir_node(ir_node->ir_lval2);
        return;
    case NODE_ASSIGN_STRING:
        printf("assign_string \t");
        print_ir_node(ir_node->ir_lval);
        print_ir_node(ir_node->ir_lval2);
        return;
    }
}

void print_all_modules() {
    int i;
    ir_node_t *module;

    for(i=0;i<MAX_NUM_OF_MODULES;i++) {
        module = ir_root_tree[i];
        if (!module) {return;}
        print_module(module);
        new_final_tree();
    }
}

void print_module(ir_node_t *module) {
    ir_node_t *ir_node;

    if (!module) {
        return;
    }

    ir_node = module;

    while(ir_node) {
        //every node can have a label
        if (!ir_node->label) {
            printf("\t\t\t");
        }

        print_ir_node(ir_node);
        ir_node = ir_node->next;
        printf("\n");
    }
}

char *op_to_instruction_literal(op_t op) {
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

mips_instr_t *op_to_mips_instr_t(op_t op, type_t datatype) {
    if (datatype==TYPE_REAL) {
        switch (op) {
        case RELOP_LE:               // '<='
        case RELOP_B:                // '>'
            return &I_c_le_s;
        case RELOP_L:                // '<'
        case RELOP_BE:               // '>='
            return &I_c_lt_s;
        case RELOP_EQU:	             // '='
        case RELOP_NE:               // '<>'
            return &I_c_eq_s;
        case OP_PLUS:                // '+'
            return &I_add_s;
        case OP_MINUS:               // '-'
            return &I_sub_s;
        case OP_MULT:                // '*'
            return &I_mul_s;
        case OP_RDIV:                // '/'
            return &I_div_s;
        case OP_AND:       	     // 'and'
            //return &I_and;
        case OP_OR:		     // 'or'
            //return &I_or;
        case OP_NOT:       	     // 'not'
            //return &I_not;
        case OP_DIV:       	     // 'div'
            //return &I_div;
        case OP_MOD:       	     // 'mod'
            //return &I_rem;
        case RELOP_IN:	// 'in'
            //return "in";
        case OP_IGNORE:
            //return "__op_IGNORE__";
        case OP_SIGN: 	//dummy operator, to determine when the the OP_PLUS, OP_MINUS are used as sign
            //return "op_SIGN";
        default:
            die("UNEXPECTED_ERROR: bad boolean operator in real ir_node");
            return NULL; //keep the compiler happy
        }
    } else {
        switch (op) {
        case RELOP_B:	// '>'
            return &I_bgt;
        case RELOP_BE:	// '>='
            return &I_bge;
        case RELOP_L:	// '<'
            return &I_blt;
        case RELOP_LE:	// '<='
            return &I_ble;
        case RELOP_NE:	// '<>'
            return &I_bne;
        case RELOP_EQU:	// '='
            return &I_beq;
        case OP_PLUS:	// '+'
            return &I_add;
        case OP_MINUS:	// '-'
            return &I_sub;
        case OP_MULT:	// '*'
            return &I_mult;
        case OP_RDIV:	// '/'
            return &I_div_s;
        case OP_DIV:       	// 'div'
            return &I_div;
        case OP_MOD:       	// 'mod'
            return &I_rem;
        case OP_AND:       	// 'and'
            return &I_and;
        case OP_OR:		// 'or'
            return &I_or;
        case OP_NOT:       	// 'not'
            return &I_not;
        case RELOP_IN:	// 'in'
            //return "in";
        case OP_IGNORE:
            //return "__op_IGNORE__";
        case OP_SIGN: 	//dummy operator, to determine when the the OP_PLUS, OP_MINUS are used as sign
            //return "op_SIGN";
        default:
            die("UNEXPECTED_ERROR: 04");
            return NULL; //keep the compiler happy
        }
    }
}
