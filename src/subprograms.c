#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "subprograms.h"
#include "subprograms_toolbox.h"
#include "symbol_table.h"
#include "mem.h"
#include "err_buff.h"
#include "statements.h"

void configure_formal_parameters(param_list_t *list,func_t *func) {
    int i;

    if (!list) {
        //printf("INFO: no formal parameters for subprogram\n");
        return;
    }

    func->param_num = MAX_PARAMS-list->param_empty;
    for (i=0;i<func->param_num;i++) {
        func->param[i] = list->param[i];
    }
}

void check_for_return_value(func_t *subprogram,statement_t *body) {
    //subprogram here is always a function
    //reminder: body is a comp statement
    if (body->_comp.first_stmt->last->return_point==0) {
        sprintf(str_err,"control reaches end of function '%s' without return value",subprogram->name);
        yyerror(str_err);
    }
}

void check_if_function_is_obsolete(func_t *subprogram) {
    //subprogram here is always a function
    if (subprogram->return_value->status_known==KNOWN_YES) {
        //we know the return value at compile time
        //mark function as obsolete
        subprogram->status = FUNC_OBSOLETE;

        //downgrade return value to hardoced expression
        subprogram->return_value->to_expr->expr_is = EXPR_HARDCODED_CONST;

        //also update its value
        subprogram->return_value->to_expr->ival = subprogram->return_value->ival;
        subprogram->return_value->to_expr->fval = subprogram->return_value->fval;
        subprogram->return_value->to_expr->cval = subprogram->return_value->cval;
    }
}

void subprogram_init(sem_t *sem_sub) {
    func_t *subprogram;

    if (!sem_sub) {
        return;
    }

    if (sem_sub->id_is==ID_PROC || sem_sub->id_is==ID_FUNC) {
        //Multiple body definitions for a subprogram are forbidden
        //for more information see the comments in semantics.h
        sprintf(str_err,"Multiple body definition for subprogram %s.",sem_sub->name);
        yyerror(str_err);
        return;
    }

    if (sem_sub->id_is!=ID_FORWARDED_PROC && sem_sub->id_is!=ID_FORWARDED_FUNC) {
        //sanity check, should never reach here
        die("UNEXPECTED ERROR 21");
    }

    subprogram = sem_sub->subprogram;

    new_statement_module(subprogram);

    configure_stack_size_and_param_lvalues(subprogram);
    declare_formal_parameters(subprogram); //declare them inside the new scope
}

void subprogram_finit(sem_t *sem_sub,statement_t *body) {
    //mark subprogram as well-declared, if needed
    if (sem_sub->id_is == ID_FORWARDED_FUNC) {
        sem_sub->id_is = ID_FUNC;
        check_for_return_value(sem_sub->subprogram,body);
        check_if_function_is_obsolete(sem_sub->subprogram);
    }
    else if (sem_sub->id_is == ID_FORWARDED_PROC) {
        sem_sub->id_is = ID_PROC;
    }

    link_statement_to_module_and_return(sem_sub->subprogram,body);
}

sem_t *declare_function_header(char *id,param_list_t *list,data_t *return_type) {
    sem_t *sem_2;
    var_t *return_value;

    //function name belongs to current scope
    sem_2 = sm_insert(id,ID_FORWARDED_FUNC);
    if (!sem_2) {
        //if the id is used before, an error is printed by sm_insert.
        yyerror("on function declaration");
        free(id); //flex strdup'ed it
        return NULL;
    }

    sem_2->subprogram = (func_t*)calloc(1,sizeof(func_t));
    sem_2->subprogram->status = FUNC_USEFULL;
    sem_2->subprogram->name = sem_2->name;
    sem_2->subprogram->stack_size = STACK_INIT_SIZE;

    if (!list) {
        yyerror("functions must have at least one parameter.");
        //continue to avoid false error messages
    }

    return_value = (var_t*)calloc(1,sizeof(var_t));
    return_value->id_is = ID_RETURN;
    return_value->datatype = return_type;
    return_value->name = sem_2->name;
    return_value->scope = sem_2->subprogram;

    return_value->to_expr = expr_version_of_variable(return_value);

    return_value->status_value = VALUE_GARBAGE;
    return_value->status_use = USE_YES;
    return_value->status_known = KNOWN_NO;

    sem_2->subprogram->return_value = return_value;
    return_value->Lvalue = mem_allocate_return_value(sem_2->subprogram);

    configure_formal_parameters(list,sem_2->subprogram);

    return sem_2;
}

sem_t *declare_procedure_header(char *id,param_list_t *list) {
    sem_t *sem_2;

    sem_2  = sm_insert(id,ID_FORWARDED_PROC);
    if (!sem_2) {
        //if the id is used before, an error is printed by sm_insert.
        yyerror("on procedure id declaration");
        free(id); //flex strdup'ed it
        return NULL;
    }

    //do something with formal parameters
    sem_2->subprogram = (func_t*)calloc(1,sizeof(func_t));
    sem_2->subprogram->status = FUNC_USEFULL;
    sem_2->subprogram->name = sem_2->name;
    sem_2->subprogram->return_value = NULL;
    sem_2->subprogram->stack_size = STACK_INIT_SIZE;

    configure_formal_parameters(list,sem_2->subprogram);

    return sem_2;
}

void forward_subprogram_declaration(sem_t *subprogram) {
    if (subprogram) {
        if (subprogram->id_is!=ID_FORWARDED_FUNC) {
            yyerror("only functions can be forwarded");
        }
    }
    else {
        //either invalid forward on non function id, or more than one forwardes in a row
        yyerror("invalid function name or multiple forwards in a row");
    }
}
