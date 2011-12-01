#ifndef _INTERMEDIATE_REPRESENTATION_H
#define _INTERMEDIATE_REPRESENTATION_H

#include "semantics.h"

//module zero is the main program
#define MAX_NUM_OF_MODULES 128
#define NEED_BACKPATCH NULL

typedef enum ir_node_type_t {
    NODE_DUMMY_LABEL,	        //empty statement, with label only
    NODE_BRANCH,
    NODE_JUMP_LINK,	        //jump and link
    NODE_JUMP,		        //jump only
    NODE_RETURN_FUNC,           //assign return value and return controll to caller
    NODE_RETURN_PROC,           //return control to caller
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
    NODE_ASM_CONVERT_TO_INT,	//if it is neccessary
    NODE_ASM_CONVERT_TO_REAL,	//if it is neccessary
    NODE_ASM_MEMCOPY,		//for assignment of identical arrays and records
    NODE_ASM_LOAD,		//load a lvalue, (converts a lvalue to rvalue), with offset (if any)
    NODE_ASM_SAVE,		//assign statement
    NODE_TYPEOF_SET_AND,	//AND for sets (operator *,OP_MULT)
    NODE_TYPEOF_SET_OR,	        //OR for sets (operator +,OP_PLUS), inverts the bits
    NODE_TYPEOF_SET_NOT,	//NOT for sets (operator -,OP_MINUS)
    NODE_LVAL,			//calculate adderss is in a register, with offset (if any)
    NODE_HARDCODED_LVAL,	//immediate calculation of memory address, no offsets here
    NODE_RVAL,
    NODE_HARDCODED_RVAL,
    NODE_INIT_NULL_SET,
    NODE_ADD_ELEM_TO_SET,       //checks first if bigger or equal to zero
    NODE_ADD_ELEM_RANGE_TO_SET, //only the values bigger or equal to zero
    NODE_CHECK_INOP_BITMAPPED,
    NODE_ASSIGN_SET,            //the set exists in its own memory or in bitmap_factory
    NODE_ASSIGN_STRING,
    NODE_ASSIGN_MEMCPY,
} ir_node_type_t;

typedef struct ir_node_t {
    ir_node_type_t node_type;
    op_t op_rval;
    int R_register;
    int return_point;

    struct ir_node_t *next_stmt;
    struct ir_node_t *prev_stmt;
    struct ir_node_t *last_stmt;

    struct ir_node_t *ir_lval;	//first lvalue to use
    struct ir_node_t *ir_lval2;	//second lvalue to use

    struct ir_node_t *ir_rval;	//first rvalue to use, (conditions inside branch nodes are rvalues)
    struct ir_node_t *ir_rval2;	//second rvalue to use
    struct ir_node_t *ir_cond;  //the condition expression for branch statements (if, for, while) and for bound checks in assignments
    struct ir_node_t *ir_lval_dest;	//destitation address, (for special node types)

    char *label;               //the label of the node
    char *jump_label;          //if not NULL, jump to this label. only for NODE_JUMP_LINK and NODE_JUMP
    char *exit_branch_label;   //nex statement of branch stement

    mem_t *lval; //used only from load immediate
    int ival;	 //hardcoded int
    float fval;	 //hardcoded real
} ir_node_t;

//we convert sets to bitmaps here, we need two places in memory
extern var_t *ll;	//left result
extern var_t *rr;	//right result
extern var_t *x;	//protected result

void init_ir();
void new_module(func_t *subprogram);
void return_to_previous_module();
void check_for_return_value(func_t *subprogram,ir_node_t *body);


ir_node_t *new_ir_node_t(ir_node_type_t node_type);
void link_stmt_to_tree(ir_node_t *new_node);
ir_node_t *link_stmt_to_stmt(ir_node_t *child,ir_node_t *parent);
ir_node_t *link_stmt_to_stmt_anyway(ir_node_t *child,ir_node_t *parent);

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
