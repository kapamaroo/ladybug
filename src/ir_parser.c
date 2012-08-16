#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "ir.h"
#include "final_code.h"
#include "err_buff.h"
#include "instruction_set.h"
#include "reg.h"
#include "expr_toolbox.h"

enum instr_req_t {
    INSTR_IMM,
    INSTR_REG
};

mips_instr_t *op_to_mips_instr_t(op_t op, type_t datatype, enum instr_req_t instr_req);

instr_t *new_instruction(char *label,mips_instr_t *mips_instr) {
    static unsigned long unique_instr_id = 0;
    instr_t *new_instr;

    new_instr = (instr_t*)calloc(1,sizeof(instr_t));

    new_instr->id = ++unique_instr_id;
    new_instr->last = new_instr;
    new_instr->label = label;
    new_instr->mips_instr = mips_instr;

    return new_instr;
}

void set_register_files_for_virtual_regs(instr_t *instr) {
    switch (instr->mips_instr->datatype) {
    case I_FLOAT:
        if (instr->Rd) {instr->Rd->is = REG_FLOAT;}
        if (instr->Rs) {instr->Rs->is = REG_FLOAT;}
        if (instr->Rt) {instr->Rt->is = REG_FLOAT;}
        break;
    case I_INT:
        //our first choice for integers is REG_CONTENT, maybe
        //we will use REG_TEMP in some of them if we are
        //out of REG_CONTENT, but now is very early to say
        if (instr->Rd) {instr->Rd->is = REG_CONTENT;}
        if (instr->Rs) {instr->Rs->is = REG_CONTENT;}
        if (instr->Rt) {instr->Rt->is = REG_CONTENT;}
        break;
    case I_FLOAT_INT:
        //only load, store
        if (instr->mips_instr->fmt==FMT_LOAD) {
            instr->Rd->is = REG_FLOAT;
            instr->Rs->is = REG_CONTENT;
        }
        else if (instr->mips_instr->fmt==FMT_STORE) {
            instr->Rs->is = REG_FLOAT;
            instr->Rt->is = REG_CONTENT;
        }
        break;
    case I_INT_FLOAT:
        //only FMT_RS_RT
        instr->Rs->is = REG_CONTENT;
        instr->Rt->is = REG_FLOAT;
        break;
    }
}

void reg_liveness_analysis(instr_t *pc) {
    /**  ATTENTION!
     * we assume that the virtual registers appear in the final code tree
     * in ascending order. we need a more sophisticated liveness analysis
     * //FIXME
     */

    if (IS_REG_VIRT(pc->Rd) &&
        !pc->Rd->live) { pc->Rd->live = pc; }        //first appearance of virtual register
    if (IS_REG_VIRT(pc->Rs)) { pc->Rs->die = pc; }   //update last appearance
    if (IS_REG_VIRT(pc->Rt)) { pc->Rt->die = pc; }   //update last appearance

}

instr_t *link_instructions(instr_t *child, instr_t *parent) {
    if (child && parent) {
        parent->last->next = child;
        child->prev = parent->last;
        parent->last = child->last;

        set_register_files_for_virtual_regs(child);
        reg_liveness_analysis(child);

        return parent;
    } else if (child) {
        return child;
    } else if (parent) {
        return parent;
    }
    return NULL;
}

void parse_ir_node(ir_node_t *ir_node) {
    ir_node_t *tmp;
    instr_t *new_instr;

    ir_node_t *ir_eff;    //effective ir_node 1
    ir_node_t *ir_eff2;   //effective ir_node 2
    enum instr_req_t instr_req;
    mips_instr_t *new_mips_instr;

    if (ir_node->label) {
        new_instr = new_instruction(ir_node->label,&I_nop);
        final_tree_current = link_instructions(new_instr,final_tree_current);
    }

    switch(ir_node->node_type) {
    case NODE_DUMMY_LABEL:
        return;
    case NODE_BRANCH:
        switch (ir_node->op_rval) {
        case OP_OR:
        case OP_AND:
            parse_ir_node(ir_node->ir_rval);
            parse_ir_node(ir_node->ir_rval2);
            break;
        case OP_NOT:
            die("UNEXPECTED ERROR: ir_parser: NODE_BRANCH_COND: not operator still alive??");
        default:
            die("UNEXPENTED_ERROR: parse branch ir");
            break;
        }
        return;
    case NODE_JUMP_LINK:
        new_instr = new_instruction(ir_node->label,&I_jal);
        new_instr->goto_label = ir_node->ir_goto->label;
        final_tree_current = link_instructions(new_instr,final_tree_current);
        return;
    case NODE_JUMP:
        new_instr = new_instruction(ir_node->label,&I_j);
        new_instr->goto_label = ir_node->ir_goto->label;
        final_tree_current = link_instructions(new_instr,final_tree_current);
        return;
    case NODE_RETURN_SUBPROGRAM:
        new_instr = new_instruction(ir_node->label,&I_jr);
        new_instr->Rs = &R_ra;
        final_tree_current = link_instructions(new_instr,final_tree_current);
        return;
    case NODE_SYSCALL:
        //prepare
        switch (ir_node->syscall_num) {
        case SVC_PRINT_INT:
        case SVC_PRINT_CHAR:
        case SVC_PRINT_STRING:
            //set the register first
            ir_node->ir_rval->reg = &R_a0;
            parse_ir_node(ir_node->ir_rval);
            break;
        case SVC_PRINT_REAL:
            //set the register first
            ir_node->ir_rval->reg = arch_fp[12];
            parse_ir_node(ir_node->ir_rval);
            break;
        case SVC_READ_INT:
        case SVC_READ_CHAR:
        case SVC_READ_REAL:
            //it is safer to parse the address first, because it may change the special
            //registers before we use them
            parse_ir_node(ir_node->address);
            break;
        case SVC_READ_STRING:
            //pass address of string
            new_instr = new_instruction(NULL,&I_move);
            new_instr->Rd = &R_a0;
            new_instr->Rs = ir_node->ir_rval->reg;
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
            new_instr->ival = ir_node->offset->ival;
            new_instr->Rs = &R_v0;
            final_tree_current = link_instructions(new_instr,final_tree_current);
            break;
        case SVC_READ_REAL:
            //save input to mem
            new_instr = new_instruction(NULL,&I_swc1);
            new_instr->Rt = ir_node->address->last->reg;
            new_instr->ival = ir_node->offset->ival;
            new_instr->Rs = arch_fp[0];
            final_tree_current = link_instructions(new_instr,final_tree_current);
            break;
        }
        break;
    case NODE_CONVERT_TO_INT:
        parse_ir_node(ir_node->ir_rval);

        new_instr = new_instruction(NULL,&I_cvt_w_s);
        new_instr->Rd = ir_node->reg;
        new_instr->Rs = ir_node->ir_rval->last->reg;
        final_tree_current = link_instructions(new_instr,final_tree_current);
        return;
    case NODE_CONVERT_TO_REAL:
        parse_ir_node(ir_node->ir_rval);

        new_instr = new_instruction(NULL,&I_cvt_s_w);
        new_instr->Rd = ir_node->reg;
        new_instr->Rs = ir_node->ir_rval->last->reg;
        final_tree_current = link_instructions(new_instr,final_tree_current);
        return;
    case NODE_MEMCPY:
        printf("__memcpy__");
        parse_ir_node(ir_node->address);
        parse_ir_node(ir_node->ir_lval);
        return;

#if (USE_PSEUDO_INSTR_LA==1)
    case NODE_LVAL_NAME:
        new_instr = new_instruction(NULL,&I_la);
        new_instr->Rd = ir_node->reg;
        new_instr->lval_name = ir_node->lval_name;

        final_tree_current = link_instructions(new_instr,final_tree_current);
        break;
#endif

    case NODE_LOAD:
        parse_ir_node(ir_node->ir_lval);

        if (ir_node->data_is==TYPE_REAL) {
            //load directly to c1
            new_instr = new_instruction(NULL,&I_lwc1);
        } else {
            new_instr = new_instruction(NULL,&I_lw);
        }

        new_instr->Rd = ir_node->reg;

        new_instr->Rs = ir_node->ir_lval->address->last->reg;
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
        parse_ir_node(ir_node->offset);
        parse_ir_node(ir_node->address);
        return;
    case NODE_RVAL:
        //sanity check
        switch (ir_node->op_rval) {
        case OP_AND:    // 'and'
        case OP_OR:     // 'or'
        case OP_NOT:    // 'not'
        case RELOP_IN:	// 'in'
            die("UNEXPECTED ERROR: ir_parser: NODE_RVAL: logical and/or/not/in operator still alive??");
        default: break;
        }

        //handle NODE_HARDCODED_RVAL cases (init to common reg to reg instr)
        instr_req = INSTR_REG;
        ir_eff = ir_node->ir_rval;    //this may be NULL in case of OP_SIGN
        ir_eff2 = ir_node->ir_rval2;  //this always exists

        //reminder: we can have only one child node (ir_rval OR ir_rval2) marked as NODE_HARDCODED_RVAL
        //see expressions.c

        if (ir_node->op_rval!=OP_SIGN) {
            //first node of prepare_stack
            switch (ir_node->ir_rval->node_type) {
            case NODE_HARDCODED_RVAL:
                if (ir_node->ir_rval2->node_type==NODE_HARDCODED_RVAL) {
                    die("INTERNAL_ERROR: both rvalues are hardcoded");
                }

                if (ir_node->op_rval==OP_PLUS || ir_node->op_rval==OP_MINUS) {
                    instr_req = INSTR_IMM;
                    //swap rvalues (always put the hardcoded in ir_eff2)
                    ir_eff2 = ir_node->ir_rval;
                    ir_eff = ir_node->ir_rval2;
                } else {
                    new_instr = new_instruction(ir_node->ir_rval->label,&I_addi);
                    new_instr->Rd = ir_node->ir_rval->reg;
                    new_instr->Rs = &R_zero;
                    new_instr->ival = ir_node->ir_rval->ival;
                    final_tree_current = link_instructions(new_instr,final_tree_current);
                }
            case NODE_RVAL_ARCH:
                break;
            default:
                tmp = ir_node->ir_rval;
                while (tmp) {
                    //if (tmp->node_type==NODE_LOAD) { break; }
                    parse_ir_node(tmp);
                    tmp = tmp->next;
                }
                break;
            }
        }

        switch (ir_node->ir_rval2->node_type) {
        case NODE_HARDCODED_RVAL:
            if (ir_node->op_rval==OP_PLUS || ir_node->op_rval==OP_MINUS) {
                instr_req = INSTR_IMM;
                //we have the values from the init code
                //ir_eff2 = ir_node->ir_rval2;
                //ir_eff = ir_node->ir_rval;
            } else {
                //we must load the hardcoded value separately
                new_instr = new_instruction(ir_node->ir_rval2->label,&I_addi);
                new_instr->Rd = ir_node->ir_rval2->reg;
                new_instr->Rs = &R_zero;
                new_instr->ival = ir_node->ir_rval2->ival;
                final_tree_current = link_instructions(new_instr,final_tree_current);
            }
        case NODE_RVAL_ARCH:
            break;
        default:
            tmp = ir_node->ir_rval2;
            while (tmp) {
                //if (tmp->node_type==NODE_LOAD) { break; }
                parse_ir_node(tmp);
                tmp = tmp->next;
            }
            break;
        }

        new_mips_instr = op_to_mips_instr_t(ir_node->op_rval,ir_node->data_is,instr_req);
        new_instr = new_instruction(NULL,new_mips_instr);

        if (instr_req==INSTR_IMM) {
            if (ir_node->op_rval!=OP_SIGN) {
                new_instr->Rs = ir_eff->last->reg;
            } else {
                new_instr->Rs = &R_zero;
            }

            if (ir_node->op_rval==OP_MINUS) {
                new_instr->ival = -ir_eff2->ival;
            } else {
                new_instr->ival = ir_eff2->ival;
            }
        } else {
            if (ir_node->op_rval!=OP_SIGN) {
                new_instr->Rs = ir_eff->last->reg;
            }

            new_instr->Rt = ir_eff2->last->reg;
        }

        //generic code

        //set label of branches
        switch (ir_node->op_rval) {
        case RELOP_L:	// '<'
        case RELOP_LE:	// '<='
        case RELOP_EQU:	// '='
            if (ir_node->data_is==TYPE_REAL) {
                final_tree_current = link_instructions(new_instr,final_tree_current);
                new_instr = new_instruction(NULL,&I_bc1t);
            }

            new_instr->goto_label = ir_node->ir_goto->label;
            break;
        case RELOP_NE:	// '<>'
        case RELOP_B:	// '>'
        case RELOP_BE:	// '>='
            if (ir_node->data_is==TYPE_REAL) {
                final_tree_current = link_instructions(new_instr,final_tree_current);
                new_instr = new_instruction(NULL,&I_bc1f);
            }

            new_instr->goto_label = ir_node->ir_goto->label;
            break;
        default:
            //only non branch instructions need the Rd
            new_instr->Rd = ir_node->reg;
            break;
        }

        final_tree_current = link_instructions(new_instr,final_tree_current);

        //if (ir_node->next) { parse_ir_node(ir_node->next); }
        return;
    case NODE_RVAL_ARCH:
        return;
    case NODE_HARDCODED_RVAL:
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
            new_instr = new_instruction(ir_node->ir_rval->label,&I_addi);
            new_instr->Rd = ir_node->ir_rval->reg;
            new_instr->Rs = &R_zero;
            new_instr->ival = ir_node->ir_rval->ival;
            final_tree_current = link_instructions(new_instr,final_tree_current);
        case NODE_RVAL_ARCH:
            break;
        default:
            tmp = ir_node->ir_rval;
            while (tmp) {
                //if (tmp->node_type==NODE_LOAD) { break; }
                parse_ir_node(tmp);
                tmp = tmp->next;
            }
            break;
        }

        //parse the address first
        parse_ir_node(ir_node->ir_lval);

        if (ir_node->data_is==TYPE_REAL) {
            //store directly from c1
            new_instr = new_instruction(NULL,&I_swc1);
        } else {
            new_instr = new_instruction(NULL,&I_sw);
        }

        new_instr->Rs = ir_node->ir_rval->last->reg;
        new_instr->Rt = ir_node->ir_lval->address->last->reg;

        new_instr->ival = ir_node->ir_lval->offset->ival;

        final_tree_current = link_instructions(new_instr,final_tree_current);
        return;
    case NODE_ASSIGN_SET:
        printf("assign_set \t");
        parse_ir_node(ir_node->ir_lval);
        parse_ir_node(ir_node->ir_lval2);
        return;
    case NODE_ASSIGN_STRING:
        printf("assign_string \t");
        parse_ir_node(ir_node->ir_lval);
        parse_ir_node(ir_node->ir_lval2);
        return;
    }
}

void parse_module(ir_node_t *module) {
    ir_node_t *ir_node;

    if (!module) {
        return;
    }

    ir_node = module;

    while(ir_node) {
        parse_ir_node(ir_node);
        ir_node = ir_node->next;
    }
}

void parse_all_modules() {
    int i;
    ir_node_t *module;

    for(i=0;i<MAX_NUM_OF_MODULES;i++) {
        module = ir_root_tree[i];
        if (!module) {return;}
        parse_module(module);
        new_final_tree();
    }
}

mips_instr_t *op_to_mips_instr_t(op_t op, type_t datatype, enum instr_req_t instr_req) {
    //there is no 'subi' instruction, return 'addi'
    //the caller has to use the negative constant
    if (instr_req==INSTR_IMM) {
        if (datatype==TYPE_REAL) {
            die("INTERNAL_ERROR: bad hardcoded instr request: TYPE_REAL");
        }

        if (op!=OP_PLUS && op!=OP_MINUS) {
            printf("debug: bad operator: '%s'\n",op_literal(op));
            die("INTERNAL_ERROR: bad hardcoded instr request: only add/sub can be used");
        }
        return &I_addi;
    }

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
