#ifndef _INTERMEDIATE_REPRESENTATION_H
#define _INTERMEDIATE_REPRESENTATION_H

#include "semantics.h"
#include "statements.h"
#include "reg.h"
#include "services.h"

typedef enum ir_node_type_t {
    NODE_DUMMY_LABEL,	        //empty statement, with label only
    NODE_BRANCH,
    NODE_JUMP_LINK,	        //jump and link
    NODE_JUMP,		        //jump only
    NODE_RETURN_SUBPROGRAM,      //return control to caller
    NODE_SYSCALL,
    NODE_CONVERT_TO_INT,	//if it is neccessary
    NODE_CONVERT_TO_REAL,	//if it is neccessary
    NODE_MEMCPY,		//for assignment of identical arrays and records
    NODE_LOAD,		        //load a NODE_LVAL
    NODE_SHIFT_LEFT,
    NODE_SHIFT_RIGHT,
    NODE_LVAL,			//memory address with possible offset
    NODE_RVAL,
    NODE_RVAL_ARCH,             //get a RVAL directly from a register (usually a REG_POINTER register) see reg.h
                                //this is like NODE_HARDCODED_RVAL, but the value is already in a register
    NODE_HARDCODED_RVAL,
    NODE_INIT_NULL_SET,
    NODE_ADD_ELEM_TO_SET,       //checks first if bigger or equal to zero
    NODE_ADD_ELEM_RANGE_TO_SET, //only the values bigger or equal to zero
    NODE_CHECK_INOP_BITMAPPED,
    NODE_ASSIGN,		//assign statement
    NODE_ASSIGN_SET,            //obsolete, the set exists in its own memory or in bitmap_factory
    NODE_ASSIGN_STRING,
} ir_node_type_t;

typedef struct ir_node_t {
    ir_node_type_t node_type;
    op_t op_rval;
    unsigned long virt_reg;
    reg_t *reg;

    char *label;               //the label of the node

    struct ir_node_t *next;
    struct ir_node_t *prev;
    struct ir_node_t *last;

    struct ir_node_t *ir_lval;	//first lvalue to use
    struct ir_node_t *ir_lval2;	//second lvalue to use

    struct ir_node_t *ir_rval;	//first rvalue to use
    struct ir_node_t *ir_rval2;	//second rvalue to use

    struct ir_node_t *ir_lval_dest;	//for TYPESET nodes we need one more lvalue;

    struct ir_node_t *address; //NODE_LVAL ititializes it
    struct ir_node_t *offset;  //this is always a HARDCODED_RVAL

    struct ir_node_t *ir_goto; //jump to this node

    type_t data_is; //standard type
    mips_service_t syscall_num; //OS service number

    int ival;	 //hardcoded int
    float fval;	 //hardcoded real
    char cval;   //hardcoded char
} ir_node_t;

extern ir_node_t *ir_root_tree[MAX_NUM_OF_MODULES];

void init_ir();

void new_ir_tree(func_t *subprogram);

ir_node_t *new_ir_node_t(ir_node_type_t node_type);
ir_node_t *link_ir_to_ir(ir_node_t *child,ir_node_t *parent);

ir_node_t *new_ir_assign(var_t *v, expr_t *l);
ir_node_t *new_ir_if(expr_t *cond,ir_node_t *true_stmt,ir_node_t *false_stmt);
ir_node_t *new_ir_while(expr_t *cond,ir_node_t *true_stmt);
ir_node_t *new_ir_for(var_t *var,iter_t *range,ir_node_t *true_stmt);
ir_node_t *new_ir_with(ir_node_t *body);
ir_node_t *new_ir_procedure_call(func_t *subprogram,expr_list_t *list);
ir_node_t *new_ir_comp_stmt(ir_node_t *body);
ir_node_t *new_ir_read(var_list_t *list);
ir_node_t *new_ir_write(expr_list_t *list);

ir_node_t *jump_and_link_to(func_t *subprogram);
ir_node_t *jump_to(ir_node_t *ir_dest);

#endif
