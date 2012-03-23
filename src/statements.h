#ifndef __STATEMENTS_H
#define __STATEMENTS_H

#include "semantics.h"

//module zero is the main program
#define MAX_NUM_OF_MODULES 128

struct statement_t;

enum StatementType {
    ST_If,
    ST_While,
    ST_For,
    ST_Call,
    ST_Assignment,
    ST_With,
    ST_Read,
    ST_Write,
    ST_BadStatement
};

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
    struct statement_t* loop;
    expr_t *condition;
};

struct statement_assignment_t {
    var_t *var;
    expr_t *expr;
};

struct statement_for_t {
    //enum IterSpaceType type;
    //expr_t *from, *to;
    iter_t *iter;
    var_t *var;
    struct statement_t *loop;
};

struct statement_call_t {
    //char *id;
    //param_t *params;
    //data_t *type;
    //int size;
    func_t *subprogram;
    expr_list_t *expr_params;
};

struct statement_with_t {
    struct statement_t *statement;
    var_t *var;
};

struct statement_read_t {
    var_list_t *var_list;
};

struct statement_write_t {
    expr_list_t *expr_list;
};

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
    };
    int return_point; //we check this to see if a function always returns a return value

    struct statement_t *next;
    struct statement_t *prev;
    struct statement_t *last;
    struct statement_t *join;
} statement_t;

extern statement_t *statement_root_module[MAX_NUM_OF_MODULES];
extern int statement_root_module_current_free;

void init_statements();

void prepare_branch_stmt(expr_t *l);

var_t *protect_guard_var(char *id);

statement_t *statement_if(expr_t *cond, statement_t *_true, statement_t *_false);

statement_t *statement_while(expr_t *cond, statement_t *loop);
statement_t *statement_assignment(var_t *v, expr_t *l);
//statement_t *statement_assignment_str(var_t *var, char *string);
statement_t *statement_for(var_t *var, iter_t *iter_space, statement_t *loop);
statement_t *statement_call(func_t *subprogram, expr_list_t *expr_params);
statement_t *statement_with(var_t *var, statement_t *statement);
statement_t *statement_read(var_list_t *var_list);
statement_t *statement_write(expr_list_t *expr_list);

statement_t *link_statements(statement_t *child, statement_t *parent);
void link_statement_to_module_and_return(func_t *subprogram, statement_t *new_statement);
void new_statement_module(func_t *subprogram);

const char* statement_type_to_string(statement_t *statement);

#endif // __STATEMENTS_H
