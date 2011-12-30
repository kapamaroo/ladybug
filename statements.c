#include <stdio.h>
#include <stdlib.h>

#include "statements.h"
#include "symbol_table.h" //ladybug's symbol table
#include "ir.h"

statement_t *statement_root_module[MAX_NUM_OF_MODULES];
int statement_root_module_current;

void init_statements() {
    int i;

    init_ir();

    for(i=0; i<MAX_NUM_OF_MODULES; i++) {
        statement_root_module[i] = NULL;
    }

    statement_root_module_current = 0;
}

statement_t *new_statement_t(enum StatementType type) {
    statement_t *new_statement;

    new_statement = (statement_t*)malloc(sizeof(struct statement_t));
    new_statement->type = type;
    new_statement->prev = NULL;
    new_statement->next = NULL;
    new_statement->last = new_statement;
    new_statement->join = NULL;
    new_statement->return_point = 0;

    return new_statement;
}

statement_t *statement_if(expr_t *condition, statement_t *_true, statement_t *_false) {
    statement_t *new_if;

    new_if = new_statement_t(ST_If);
    new_if->_if.condition = condition;
    new_if->_if._true = _true;
    new_if->_if._true = _true;

    //propagate return_point to the last statement
    if (_true->last->return_point && _false->last->return_point) {
        new_if->return_point = 1;
    }

    return new_if;
}

statement_t *statement_while(expr_t *condition, statement_t *loop) {
    statement_t *new_while;

    new_while = new_statement_t(ST_While);
    new_while->_while.condition = condition;
    new_while->_while.loop = loop;

    return new_while;
}

statement_t *statement_assignment(var_t *var, expr_t *expr) {
    statement_t *new_assign;

    new_assign = new_statement_t(ST_Assignment);

    if (expr->expr_is==EXPR_STRING) {
        new_assign->_assignment.type = AT_String;
        new_assign->_assignment.string = expr->cstr;
    } else {
        new_assign->_assignment.type = AT_Expression;
        new_assign->_assignment.expr = expr;

        if (var->id_is==ID_RETURN) {
            new_assign->return_point = 1;
        }
    }

    return new_assign;
}

//statement_t *statement_assignment_str(var_t *var, char *string) {}

statement_t *statement_for(var_t *var, iter_t *iter_space, statement_t *loop) {
    statement_t *new_for;

    if (!var) {
        //bad for
        return new_statement_t(ST_BadStatement);
    }

    new_for = new_statement_t(ST_For);
    new_for->_for.var = var;
    new_for->_for.iter = iter_space;
    new_for->_for.loop = loop;

    return new_for;
}

statement_t *statement_call(func_t *subprogram, expr_list_t *expr_params) {
    statement_t *new_call;

    if (!subprogram) {
        //bad call
        return new_statement_t(ST_BadStatement);
    }

    new_call = new_statement_t(ST_Call);
    new_call->_call.subprogram = subprogram;
    new_call->_call.expr_params = expr_params;

    return new_call;
}

statement_t *statement_with(var_t *var, statement_t *statement) {
    statement_t *new_with;

    new_with = new_statement_t(ST_With);
    new_with->_with.var = var;
    new_with->_with.statement = statement;

    return new_with;
}

statement_t *statement_read(var_list_t *var_list) {
    statement_t *new_read;

    new_read = new_statement_t(ST_Read);
    new_read->_read.var_list = var_list;

    return new_read;
}

statement_t *statement_write(expr_list_t *expr_list) {
    statement_t *new_write;

    new_write = new_statement_t(ST_Write);
    new_write->_write.expr_list = expr_list;

    return new_write;
}

statement_t *link_statements(statement_t *child, statement_t *parent) {
    //return the head of the linked list
    if (parent && child) {
        //if child's return_point is not set, propagate the parent's return_point value
        if (child->last->return_point==0) {
            child->last->return_point = parent->last->return_point;

        }
        parent->last->next = child;
        child->prev = parent->last;
        parent->last = child->last;
        return parent;
    } else if (!parent) {
        return child;
    } else {
        return parent;
    }
}

void link_statement_to_module_and_return(statement_t *new_statement) {
    statement_root_module[statement_root_module_current] =
        link_statements(new_statement,statement_root_module[statement_root_module_current]);

    return_to_previous_ir_tree();

    if (statement_root_module_current==0) {
        return;
    }

    statement_root_module_current--;
}

void new_statement_module(char *label) {
    statement_root_module_current++;
    new_ir_tree(label);
}

//const char* statement_type_to_string(statement_t *statement);
