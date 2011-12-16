
#include "final_code.h"

void generate_final_code() {

}

code_rep *insert_in_code(code_rep *start,inst_t *node){
    if(!start || node){
        printf("Error in insert in code File final_code.c\n ");
        return NULL;
    }
    code_rep *new_node = (code_rep*) malloc (sizeof(code_rep));
    start->next = new_node;
    new_node->next = NULL;
    new_node->node = node;
    return new_node;
}



int travel_nodes(ir_node_t *start,code_rep *node){
    if( !start ){
        return (-1) ;
    }
    int r1,r2,r3,r4;
    inst_t *new_node;
    r1 = travel_nodes(start->ir_lval,node);
    r2 = travel_nodes(start->ir_lval2,node);
    r3 = travel_nodes(start->ir_rval,node);
    r4 = travel_nodes(start->ir_rval2,node);

    new_node = create_code(start,r1,r2,r3,r4);
    node = insert_in_code(node,new_node);
    if(!node){
        printf("ERROR in travel_nodes FILE final_code.c\n")
            exit(0);
    }

    travel_nodes(start->next_stmt,node);
    return start->R_register;
}

inst_t *create_code(ir_node_t *node,int r1,int r2,int r3,int r4){
    if(!node){
        printf("ERROR in create code FILE final_code.c\n")
            exit(0);
    }
    char *buf = (char *) malloc (5 * sizeof(char));
    inst_t *new_inst;
    int rl,rr;
    new_inst = (inst_t*) malloc sizeof((inst_t));

    switch (start->node_type){
    case NODE_DUMMY_LABEL:
	new_inst->label = node->label;
        break;
    case NODE_BRANCH:
	new_inst->name = Bne;
	new_inst->r2 = 0;
	new_inst->r3 = r1;
	itoa(start->ir_id,buf,10);
	new_inst->label = buf;
        break;
    case NODE_JUMP_LINK:
        new_inst->name = Jal;
        new_inst->label = node->label;
        new_isnt->r1 = node->R_register;
        break;
    case NODE_JUMP:
        new_inst->name = Jump;
        new_inst->label = node->label;
        break;
        //    case NODE_RETURN_FUNC:
        //      break;
        //    case NODE_RETURN_PROC:
        //      break;
        //    case NODE_INPUT_INT:
        //      break;
        //    case NODE_INPUT_REAL:
        //      break;
        //    case NODE_INPUT_BOOLEAN:
        //      break;
        //    case NODE_INPUT_CHAR:
        //      break;
        //    case NODE_INPUT_STRING:
        //      break;
        //    case NODE_OUTPUT_INT:
        //     break;
        //    case NODE_OUTPUT_REAL:
        //      break;
        //    case NODE_OUTPUT_BOOLEAN:
        //     break;
        //    case NODE_OUTPUT_CHAR:
        //      break;
        //    case NODE_OUTPUT_STRING:
        //      break;
        //    case NODE_ASM_CONVERT_TO_INT :
        //      break;
        //    case NODE_ASM_CONVERT_TO_REAL :
        //      break;
        //    case NODE_ASM_MEMCOPY :
        //      break;
    case NODE_ASM_LOAD :
        new_inst->name = Lw;
        new_isnt->r1 = node->R_register;
        new_inst->r2 = r1;
        new_inst->imediate_value = 0;
        break;
    case NODE_ASM_SAVE :
        new_inst->name = Sw;
        new_isnt->r1 = node->R_register;
        new_inst->r2 = r1;
        new_inst->imediate_value = 0;
        break;
        //    case NODE_TYPEOF_SET_AND:
        //      break;
        //    case NODE_TYPEOF_SET_OR:
        //      break;
        //    case NODE_TYPEOF_SET_NOT:
        break;
    case NODE_LVAL :
        new_inst->name = addi;
        new_isnt->r1 = node->R_register;
        new_inst->r2 = 1;
        new_inst->imediate_value = node->lval->seg_offset;
        break;
        //    case NODE_HARDCODED_LVAL:
        //      break;
    case NODE_RVAL :
        new_inst->name = find_instruction(node,&r1,&r2);
        new_isnt->r1 = node->R_register;
        new_inst->r2 = r1;
        new_inst->r3= r2;
        break;
    case NODE_HARDCODED_RVAL :
        break;
        //    case NODE_INIT_NULL_SET :
        //      break;
        //    case NODE_ADD_ELEM_TO_SET:
        //      break;
        //    case NODE_ADD_ELEM_RANGE_TO_SET:
        //      break;
        //    case NODE_CHECK_INOP_BITMAPPED :
        //      break;
        //    case NODE_ASSIGN_SET:
        //      break;
        //    case NODE_ASSIGN_STRING:
        //      break;
        //    case NODE_ASSIGN_MEMCPY:
        //      break;
    }
}
