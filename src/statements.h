#ifndef __STATEMENTS_H
#define __STATEMENTS_H

#include "semantics.h"
#include "statistics.h"

//module zero is the main program
#define MAX_NUM_OF_MODULES 128

struct statement_t;

enum StatementType {
    ST_BadStatement,

    //blocks
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

#define NEW_STMT_BLOCK_STARTS_FROM(s) (s->type==ST_If || s->type==ST_While || \
                                       s->type==ST_For || s->type==ST_Comp)

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
    struct statement_t *prologue;
    struct statement_t* loop;
    struct statement_t *epilogue;
    expr_t *condition;
};

struct statement_assignment_t {
    var_t *var;
    expr_t *expr;
};

struct statement_for_t {
    //enum IterSpaceType type;
    //expr_t *from, *to;

    int unroll_me;

    iter_t *iter;
    var_t *var;
    struct statement_t *prologue;
    struct statement_t *loop;
    struct statement_t *epilogue;
};

struct statement_call_t {
    func_t *subprogram;
    expr_list_t *expr_params;
};

struct statement_with_t {
    struct statement_t *body;
    var_t *var;
};

struct statement_read_t {
    var_list_t *var_list;
};

struct statement_write_t {
    expr_list_t *expr_list;
};

struct statement_comp_t {
    struct statement_t *first_stmt;
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
        struct statement_comp_t _comp;
    };

    //statistics
    stat_vars_t stats_of_vars;
    dep_vector_t *dep;

    int return_point; //we check this to see if a function always returns a return value

    struct statement_t *next;
    struct statement_t *prev;
    struct statement_t *last;
    struct statement_t *join;
} statement_t;

extern statement_t *statement_root_module[MAX_NUM_OF_MODULES];
extern int statement_root_module_current_free;

extern unsigned int inside_branch;
extern unsigned int inside_loop;

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
statement_t *statement_comp(statement_t *first_stmt);

statement_t *link_statements(statement_t *child, statement_t *parent);
void link_statement_to_module_and_return(func_t *subprogram, statement_t *new_statement);
void new_statement_module(func_t *subprogram);

#endif // __STATEMENTS_H
