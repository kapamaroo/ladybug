#include <stdio.h>
#include <stdlib.h>

#include "statements.h"
#include "symbol_table.h" //ladybug's symbol table
#include "datatypes.h"
#include "scope.h"
#include "expressions.h" //maybe we need to invert the cond in if
#include "expr_toolbox.h"
#include "ir.h"
#include "err_buff.h"

statement_t *statement_root_module[MAX_NUM_OF_MODULES];
int statement_root_module_current_free;
unsigned int inside_branch_stmt = 0;

void init_statements() {
    int i;

    for(i=0; i<MAX_NUM_OF_MODULES; i++) {
        statement_root_module[i] = NULL;
    }

    statement_root_module_current_free = 0;
}

statement_t *link_statements(statement_t *child, statement_t *parent) {
    //return the head of the linked list
    if (parent && child) {
        //if child's return_point is not set, propagate the parent's return_point value
        if (child->last->return_point==0) {
            child->last->return_point = parent->last->return_point;

        }

        parent->last->join = child;
        if (parent->type==ST_If) {
            parent->_if._true->last->join = child;
            if (parent->_if._false) {
                parent->_if._false->last->join = child;
            }
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

void link_statement_to_module_and_return(func_t *subprogram, statement_t *new_statement) {
    close_scope(subprogram);

#warning we leak memory on obsolete functions //FIXME
    if (subprogram->status==FUNC_OBSOLETE) { return; }

    statement_root_module[subprogram->unique_id] =
        link_statements(new_statement,statement_root_module[subprogram->unique_id]);
}

void new_statement_module(func_t *subprogram) {
    if (statement_root_module_current_free==MAX_NUM_OF_MODULES) {
        die("CANNOT HANDLE SO MUCH MODULES YET");
    }

    subprogram->unique_id = statement_root_module_current_free++;
    start_new_scope(subprogram);
    new_ir_tree(subprogram);
}

int check_assign_similar_comp_datatypes(data_t* vd, data_t* ld){
    int i;

    //explicit datatype matching
    if (vd==ld) {
        return 1;
    }

    //synonym datatype matching
    if (vd->is==ld->is) {
        return 1;
    }
    else if (vd->is==TYPE_BOOLEAN) {
        //we can only assign booleans to booleans
        return 0;
    }

    //compatible datatype matching
    if (TYPE_IS_STRING(vd) && TYPE_IS_STRING(ld)) {
        return 1;
    }
    else if (TYPE_IS_ARITHMETIC(vd) && TYPE_IS_ARITHMETIC(ld)) {
        return 1;
    }
    else if (vd->is==TYPE_SUBSET) {
        return check_assign_similar_comp_datatypes(vd->def_datatype,ld);
    }
    else if (vd->is==ld->is && vd->is==TYPE_ARRAY) {
        return check_assign_similar_comp_datatypes(vd->def_datatype,ld->def_datatype);
    }
    else if (vd->is==TYPE_SET) {
        //do not recurse here, we want only explicit datatype matching
        if (vd->def_datatype==ld) {
            return 1;
        }
        return 0;
    }
    else if (vd->is==ld->is && vd->is==TYPE_RECORD) {
        if (vd->field_num==ld->field_num) {
            for(i=0;i<vd->field_num;i++) {
                if (!check_assign_similar_comp_datatypes(vd->field_datatype[i],
                                                         ld->field_datatype[i])) {
                    return 0;
                }
            }
            return 1;
        }
        return 0;
    }
    else {
        return 0;
    }
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

statement_t *statement_if(expr_t *cond, statement_t *_true, statement_t *_false) {
    statement_t *new_if;

    if (!cond) {
        die("UNEXPECTED_ERROR: 63-1");
    }

    if (cond->expr_is==EXPR_LOST || cond->datatype->is!=TYPE_BOOLEAN) {
        //parse errors or empty if statement, ignore statement
        return new_statement_t(ST_BadStatement);
    }

    if (!_true && !_false) {
        //ignore empty if
        return NULL;
    }

    if (!_true && _false) {
        //revert the condition
        cond = expr_orop_andop_notop(NULL,OP_NOT,cond);
        _true = _false;
        _false = NULL;
    }

    //else (_true && _false) or (_true && !_false)
    //we do this for for the low level tree, which assumes that always there is the _true
    //it is our job as high level to prepare the if statement

    if (cond->expr_is==EXPR_HARDCODED_CONST) {
        //ignore if statement, we already know the result
        if (cond->ival) {
            return _true;  //TRUE
        } else {
            return _false; //FALSE
        }
    }

    new_if = new_statement_t(ST_If);

    //_true->prev = new_if;
    //_false->prev = new_if;

    new_if->_if.condition = cond;
    new_if->_if._true = _true;
    new_if->_if._false = _false;

    //propagate return_point to the last statement
    if (_true && _false && _true->last->return_point && _false->last->return_point) {
        new_if->return_point = 1;
    }

    inside_branch_stmt--;

    return new_if;
}

statement_t *statement_while(expr_t *cond, statement_t *loop) {
    statement_t *new_while;

    if (!cond) {
        die("UNEXPECTED_ERROR: 63-5");
    }

    if (cond->expr_is==EXPR_LOST) {
        //parse errors or empty while statement, ignore statement
        return new_statement_t(ST_BadStatement);
    }

    if (!loop) {
        //ignore empty while
        return NULL;
    }

    new_while = new_statement_t(ST_While);
    new_while->_while.condition = cond;
    new_while->_while.loop = loop;

    inside_branch_stmt--;

    return new_while;
}

statement_t *statement_assignment(var_t *v, expr_t *l) {
    statement_t *new_assign;

    func_t *scope_owner;

    if (!v || !l) {
        //should never reach here
        die("UNEXPECTED ERROR: null expression in assignment");
    }

    //check for valid assignment
    if (v->id_is==ID_LOST || l->expr_is==EXPR_LOST) {
        //parse errors
        return new_statement_t(ST_BadStatement);
    }

    if (v->id_is == ID_VAR_GUARDED) {
        sprintf(str_err,"forbidden assignment to '%s' which controls a `for` statement",v->name);
        yyerror(str_err);
        return new_statement_t(ST_BadStatement);
    }

    if (v->id_is != ID_RETURN && v->id_is != ID_VAR) {
        sprintf(str_err,"trying to assign to symbol '%s' which is not a variable",v->name);
        yyerror(str_err);
        return new_statement_t(ST_BadStatement);
    }

    scope_owner = get_current_scope_owner();

    if (v->id_is==ID_RETURN) {
        if (v->scope!=scope_owner) {
            //v->name is the same with function's name because that's how functions return their value
            sprintf(str_err,"subprogram '%s' assigns return value of function '%s'" ,scope_owner->name, v->name);
            yyerror(str_err);
            return new_statement_t(ST_BadStatement);
        }
    }

    //reminder: expressions hava standard datatypes or set datatype
    if (!check_assign_similar_comp_datatypes(v->datatype,l->datatype)) {
        sprintf(str_err,"assignment to '%s' of type '%s' with type '%s'",v->name,v->datatype->name,l->datatype->name);
        yyerror(str_err);
        return new_statement_t(ST_BadStatement);
    }

    //set status_known
    //ONLY for native variables in scope
    //ONLY when outside if,for,while statements //FIXME
    //reminder: known variables become hardcoded constants, see expr_toolbox.c
    if (l->expr_is == EXPR_HARDCODED_CONST && !inside_branch_stmt && v->scope==scope_owner) {
        v->status_known = KNOWN_YES;
    } else {
        v->status_known = KNOWN_NO;
    }

    if (v->status_known == KNOWN_YES) {
        switch (l->datatype->is) {
        case TYPE_INT:      v->ival = l->ival;  break;
        case TYPE_REAL:     v->fval = l->fval;  break;
        case TYPE_CHAR:     //reminder: char and boolean types both internally are represented as chars
        case TYPE_BOOLEAN:  v->cval = l->cval;
        }
    }

    //skip first init with value known at compile time, EXCEPT for return_values
    //this will go to the .data segment
#warning actually DO the .data segment
    if (v->id_is!=ID_RETURN &&
        v->scope==main_program &&           //ONLY main program variables can be in .data segment
        v->status_value==VALUE_GARBAGE &&
        v->status_use==USE_NONE &&
        v->status_known==KNOWN_YES) {
        v->status_value = VALUE_VALID;
        //printf("debug: first unused assignement to '%s' in '%s' goes to '.data' segment\n", v->name, scope_owner->name);
        return NULL;
    }

    v->status_value = VALUE_VALID;

    new_assign = new_statement_t(ST_Assignment);
    new_assign->_assignment.var = v;
    new_assign->_assignment.expr = l;

    if (v->id_is==ID_RETURN) {
        //printf("debug: return name: %s\tknown:%d\n", v->name, v->status_known);
        new_assign->return_point = 1;
    }

    return new_assign;
}

//statement_t *statement_assignment_str(var_t *var, char *string) {}

statement_t *statement_for(var_t *var, iter_t *iter_space, statement_t *loop) {
    statement_t *new_for;

    if (!var || !iter_space) {
        //bad for
        return new_statement_t(ST_BadStatement);
    }

    if (var->id_is!=ID_VAR_GUARDED) {
        die("INTERNAL_ERROR: variable is not a guard variable, check 'new_for_statement()'");
    }

    if (!loop) {
        //ignore empty for
        return NULL;
    }

    new_for = new_statement_t(ST_For);
    new_for->_for.var = var;
    new_for->_for.iter = iter_space;
    new_for->_for.loop = loop;

    return new_for;
}

statement_t *statement_call(func_t *subprogram, expr_list_t *expr_params) {
    statement_t *new_call;

    if (!subprogram || !check_valid_subprogram_call(subprogram,expr_params)) {
        //bad call
        return new_statement_t(ST_BadStatement);
    }

    //printf("debug: procedure call: %s\n", subprogram->name);

    new_call = new_statement_t(ST_Call);
    new_call->_call.subprogram = subprogram;
    new_call->_call.expr_params = expr_params;

    return new_call;
}

statement_t *statement_with(var_t *var, statement_t *statement) {
    statement_t *new_with;

    if (!var) {
        return new_statement_t(ST_BadStatement);
    }

    if (!statement) {
        //ignore empty with
        return NULL;
    }

    new_with = new_statement_t(ST_With);
    new_with->_with.var = var;
    new_with->_with.statement = statement;

    return new_with;
}

statement_t *statement_read(var_list_t *var_list) {
    statement_t *new_read;
    int i;
    int error=0;
    var_t *v;

    //print every possible error (if any) before returning NULL
    for(i=0;i<MAX_VAR_LIST-var_list->var_list_empty;i++) {
        v = var_list->var_list[i];
        if (v->id_is==ID_LOST) {
            error++;
        } else if (v->id_is==ID_VAR_GUARDED) {
            sprintf(str_err,"in read, control variable '%s' of 'for statement' is read only",v->name);
            yyerror(str_err);
            error++;
        } else if (v->id_is==ID_CONST) {
            sprintf(str_err,"in read, trying to change constant '%s'",v->name);
            yyerror(str_err);
            error++;
        } else if (v->id_is==ID_RETURN) {
            sprintf(str_err,"in read, return value '%s' can be set only with assignment",v->name);
            yyerror(str_err);
            error++;
        } else if (!TYPE_IS_STANDARD(v->datatype) && !TYPE_IS_STRING(v->datatype)) {
            sprintf(str_err,"in read, expected standard type '%s' or string, istead of '%s'",v->name,v->datatype->name);
            yyerror(str_err);
            error++;
        }
    }

    if (error) {
        return new_statement_t(ST_BadStatement);
    }

    //mark variables as unknown
    for(i=0;i<MAX_VAR_LIST-var_list->var_list_empty;i++) {
        v = var_list->var_list[i];
        v->status_known = KNOWN_NO;
        v->status_value = VALUE_VALID;
        //do not touch status_use
        //v->statis_use = USE_YES;
    }

    new_read = new_statement_t(ST_Read);
    new_read->_read.var_list = var_list;

    return new_read;
}

statement_t *statement_write(expr_list_t *expr_list) {
    statement_t *new_write;
    int i;
    int error=0;
    expr_t *l;

    //print every possible error
    for(i=0;i<MAX_EXPR_LIST-expr_list->expr_list_empty;i++) {
        l = expr_list->expr_list[i];
        if (l->expr_is==EXPR_LOST) {
            error++;
        }
        else if (l->expr_is==EXPR_SET || EXPR_NULL_SET) {
            sprintf(str_err,"in write, can only print standard types or strings, this is a set");
            yyerror(str_err);
            error++;
        }
        else if (!TYPE_IS_STANDARD(l->datatype) && l->expr_is!=EXPR_STRING) {
            //EXPR_RVAL, EXPR_HARDCODED_CONST, EXPR_LVAL, EXPR_STRING
            sprintf(str_err,"in write, '%s' is not standard type ('%s') or string",l->var->name,l->datatype->name);
            yyerror(str_err);
            error++;
        }
    }

    if (error) {
        return new_statement_t(ST_BadStatement);
    }

    new_write = new_statement_t(ST_Write);
    new_write->_write.expr_list = expr_list;

    return new_write;
}

void prepare_branch_stmt(expr_t *l) {
    if (!l) {
        die("UNEXPECTED_ERROR: 03");
    }

    if (l->datatype->is!=TYPE_BOOLEAN) {
        yyerror("a flow control expression must be boolean");
    }

    inside_branch_stmt++;
}

var_t *protect_guard_var(char *id) {
    sem_t *sem_2;

    inside_branch_stmt++;

    sem_2 = sm_find(id);
    if (sem_2) {
        if (sem_2->id_is==ID_VAR) {
            if (sem_2->var->datatype->is==TYPE_INT) {
                sem_2->var->id_is = ID_VAR_GUARDED;
                return sem_2->var;
            }
            else {
                sprintf(str_err,"control variable '%s' must be integer",id);
                yyerror(str_err);
            }
        }
        else if (sem_2->id_is==ID_VAR_GUARDED) {
            sprintf(str_err,"variable '%s' already controls a for statement",id);
            yyerror(str_err);
        }
        else {
            sprintf(str_err,"invalid reference to '%s', expected variable",id);
            yyerror(str_err);
        }
    }
    //else
    //nothing, error is printed from sm_insert
    //yyerror("the name of the control variable is declared before in this scope");
    return NULL;
}

void unprotect_guard_var(var_t *var) {
    inside_branch_stmt--;

    if (var) {
        var->id_is = ID_VAR;
    }
}
