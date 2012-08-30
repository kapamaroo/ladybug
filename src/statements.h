#ifndef __STATEMENTS_H
#define __STATEMENTS_H

#include "semantics.h"
#include "statistics.h"

//High Level representation of source code (Front End)

//module zero is the main program
#define MAX_NUM_OF_MODULES 128

//predefinition of struct
struct statement_t;

//Global Front End Module Buffer
extern struct statement_t *statement_root_module[MAX_NUM_OF_MODULES];
extern int statement_root_module_current_free;

//sentinels for constant/known-value propagation handling
//sometimes it is not safe to propagate values
//play safe in these cases
extern unsigned int inside_branch;  //has nonzero value when inside if statement body
extern unsigned int inside_loop;    //has nonzero value when inside loop statement

//possible types of statements
enum StatementType {
    ST_BadStatement,  //the parser generates this type of statement on errors
                      //mostly for debugging. If there is at least 1 error,
                      //we abord the compilation and the Front End colapses

    //blocks (statements that define a new logical block),
    //!!! these are NOT SSA blocks !!! we have different semantics
    ST_If,
    ST_While,
    ST_For,
    ST_Comp,

    //virtual statement, it is more of a scope modifier
    ST_With,

    //primitive statement
    ST_Assignment,

    //function calls, consider them as blocks too
    ST_Call,

    //system calls, consider them as blocks too
    ST_Read,
    ST_Write,
};

//macro to recognize block entry points
#warning make this an inline function  //FIXME
#define NEW_STMT_BLOCK_STARTS_FROM(s) (s->type==ST_If || s->type==ST_While || \
                                       s->type==ST_For || s->type==ST_Comp)
//for loop iteration range type
enum IterSpaceType {
    FT_DownTo,
    FT_To
};

struct statement_if_t {
    struct statement_t *_true;
    struct statement_t *_false;
    expr_t *condition;
};

struct statement_while_t {
    //reminder: currently there is no optimization for while statements,
    //          leaving the prologue/epiloge NULL

    struct statement_t *prologue;  //used from loop optimizers
    struct statement_t *loop;      //main loop body
    struct statement_t *epilogue;  //used from loop optimizers
    expr_t *condition;             //boolean condition
};

struct statement_assignment_t {
    var_t *var;
    expr_t *expr;
};

struct statement_for_t {
    int unroll_me;  //set to 1 for well defined, opt friendly loops
                    //else set to 0

    iter_t *iter;   //iter range
    var_t *var;     //for loop iterator (guard var)

    struct statement_t *prologue;  //used from loop optimizers
    struct statement_t *loop;      //main loop body
    struct statement_t *epilogue;  //used from loop optimizers
};

struct statement_call_t {
    func_t *subprogram;        //which subprogram to call
    expr_list_t *expr_params;  //args of subprogram (if any)
};

struct statement_with_t {
    struct statement_t *body;  //main loop body
    var_t *var;                //variable of type RECORD
};

struct statement_read_t {
    var_list_t *var_list;  //read statement's list
};

struct statement_write_t {
    expr_list_t *expr_list;  //write statement's list
};

struct statement_comp_t {
    struct statement_t *head;  //comp statement entry indicator
};

//definition of statement_t, considering all possible statement types
//the same struct is used as block container, this means that some
//statements are interpreted internally as logical blocks, see above
typedef struct statement_t {
    enum StatementType type;
    union {
        struct statement_if_t _if;
        struct statement_while_t _while;
        struct statement_assignment_t _assignment;
        struct statement_for_t _for;
        struct statement_call_t _call;
        struct statement_with_t _with;
        struct statement_read_t _read;
        struct statement_write_t _write;
        struct statement_comp_t _comp;
    };

    //keep statistics (common in both statement and block instances)
    stat_vars_t io_vectors;
    dep_vector_t *dep;

    //only for block instances  //FIXME
    unsigned int size;   //num of statements (high level)
    unsigned int depth;  //depth of nested block, outermost loop has depth=0

    int stat_id;  //unique id of statement inside the parent block
                  //implemented as simple position-in-the-list index

    int return_point;  //set to 1 if the parent block returns control
                       //to the caller subprogram/module
                       //else set to 0

    //double linked list pointers
    struct statement_t *next;
    struct statement_t *prev;
    struct statement_t *last;  //used to speed append/concatenation of lists
                               //must be used ONLY from the head of the list
                               //all other statements have undefined value

    struct statement_t *join;  //similar to *last, but for if statement's
                               //flow control manipulation
} statement_t;

void init_statements();

void prepare_if_stmt(expr_t *l);
void prepare_while_stmt(expr_t *l);
var_t *prepare_for_stmt(char *id);

statement_t *statement_if(expr_t *cond, statement_t *_true, statement_t *_false);
statement_t *statement_while(expr_t *cond, statement_t *loop);
statement_t *statement_assignment(var_t *v, expr_t *l);
statement_t *statement_for(var_t *var, iter_t *iter_space, statement_t *loop);
statement_t *statement_call(func_t *subprogram, expr_list_t *expr_params);
statement_t *statement_with(var_t *var, statement_t *body);
statement_t *statement_read(var_list_t *var_list);
statement_t *statement_write(expr_list_t *expr_list);
statement_t *statement_comp(statement_t *head);

statement_t *link_statements(statement_t *child, statement_t *parent);
statement_t *unlink_statement(statement_t *s, statement_t *head);
void link_statement_to_module_and_return(func_t *subprogram, statement_t *new_statement);
void new_statement_module(func_t *subprogram);

#endif // __STATEMENTS_H
