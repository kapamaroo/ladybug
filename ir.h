#ifndef _INTERMEDIATE_REPRESENTATION_H
#define _INTERMEDIATE_REPRESENTATION_H

#include "semantics.h"
#include "statements.h"
#include "reg.h"

#define NEED_BACKPATCH NULL

typedef enum ir_node_type_t {
    NODE_DUMMY_LABEL,	        //empty statement, with label only
    NODE_LOST_NODE,              //on parse errors generate this node
    NODE_BRANCH,
    NODE_JUMP_LINK,	        //jump and link
    NODE_JUMP,		        //jump only
    NODE_RETURN_SUBPROGRAM,      //return control to caller
    NODE_INPUT_INT,
    NODE_INPUT_REAL,
    NODE_INPUT_BOOLEAN,         //set non zero values as true
    NODE_INPUT_CHAR,            //read character (service 12)
    NODE_INPUT_STRING,
    NODE_OUTPUT_INT,            //print integers and booleans
    NODE_OUTPUT_REAL,
    NODE_OUTPUT_BOOLEAN,
    NODE_OUTPUT_CHAR,           //print character (service 11)
    NODE_OUTPUT_STRING,
    NODE_CONVERT_TO_INT,	//if it is neccessary
    NODE_CONVERT_TO_REAL,	//if it is neccessary
    NODE_MEMCPY,		//for assignment of identical arrays and records
    NODE_LOAD,		        //load a NODE_LVAL or NODE_HARDCODED_LVAL
    NODE_BINARY_AND,	        //AND for sets (operator *,OP_MULT)
    NODE_BINARY_OR,	        //OR for sets (operator +,OP_PLUS), inverts the bits
    NODE_BINARY_NOT,    	//NOT for sets (operator -,OP_MINUS)
    NODE_SHIFT_LEFT,
    NODE_SHIFT_RIGHT,
    NODE_LVAL,			//memory address with possible offset
    NODE_HARDCODED_LVAL,	//immediate memory address, with possible offset
    NODE_RVAL,
    NODE_RVAL_ARCH,             //get a RVAL directly from a register (usually a REG_POINTER register) see reg.h
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

    struct ir_node_t *next;
    struct ir_node_t *prev;
    struct ir_node_t *last;

    struct ir_node_t *ir_lval;	//first lvalue to use
    struct ir_node_t *ir_lval2;	//second lvalue to use

    struct ir_node_t *ir_rval;	//first rvalue to use
    struct ir_node_t *ir_rval2;	//second rvalue to use

    struct ir_node_t *ir_cond;  //(if, for, while), bound checks

    struct ir_node_t *ir_lval_dest;	//for TYPESET nodes we need one more lvalue;

    struct ir_node_t *address; //NODE_HARDCODED_LVAL,NODE_LVAL ititializes it, NODE_LOAD,NODE_ASSIGN* reads it
    struct ir_node_t *offset;  //NODE_LOAD adds this to *address

    char *label;               //the label of the node
    char *jump_label;          //only for NODE_JUMP_LINK, NODE_JUMP, NODE_BRANCH

    char *error; //NODE_LOST_NODE uses this

    type_t data_is; //standard type
    mem_t *lval; //generate *address and *offset from here (this should be removed)
    int ival;	 //hardcoded int
    float fval;	 //hardcoded real
    char cval;   //hardcoded char
} ir_node_t;

extern ir_node_t *ir_root_module[MAX_NUM_OF_MODULES];
extern int ir_root_module_empty;
extern int ir_root_module_current;
extern ir_node_t *ir_root;

void init_ir();
void new_module(func_t *subprogram);
void return_to_previous_module();

ir_node_t *new_ir_node_t(ir_node_type_t node_type);
void link_stmt_to_tree(ir_node_t *new_node);
ir_node_t *link_stmt_to_stmt(ir_node_t *child,ir_node_t *parent);

ir_node_t *new_assign_stmt(var_t *v, expr_t *l);
ir_node_t *new_if_stmt(expr_t *cond,ir_node_t *true_stmt,ir_node_t *false_stmt);
ir_node_t *new_while_stmt(expr_t *cond,ir_node_t *true_stmt);
ir_node_t *new_for_stmt(char *guard_var,iter_t *range,ir_node_t *true_stmt);
ir_node_t *new_with_stmt(ir_node_t *body);
ir_node_t *new_procedure_call(char *id,expr_list_t *list);
ir_node_t *new_comp_stmt(ir_node_t *body);
//ir_node_t *new_io_statement is divided to read and write
ir_node_t *new_read_stmt(var_list_t *list);
ir_node_t *new_write_stmt(expr_list_t *list);

ir_node_t *jump_and_link_to(char *jump_label);
ir_node_t *jump_to(char *jump_label);

#endif
