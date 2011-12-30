#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "subprograms.h"
#include "scope.h"
#include "symbol_table.h"
#include "mem.h"
#include "err_buff.h"
#include "statements.h"

char *new_label_subprogram(char *sub_name) {
    //char label_buf[MAX_LABEL_SIZE];

    //snprintf(label_buf,MAX_LABEL_SIZE,"S_%s",sub_name);
    //return strdup(label_buf);
    return strdup(sub_name);
}

param_list_t *param_insert(param_list_t *new_list,pass_t mode,data_t *type) {
    int i;
    param_t *new_param;
    param_list_t *list;

    if (!new_list) {
        list = (param_list_t*)malloc(sizeof(param_list_t));
        list->param_empty = MAX_PARAMS;
    }
    else {
        list = new_list;
    }

    /* enable this if we want to declare parameters when their type isn't declared right,
     * to avoid unreal error messages
     */
    if (!type) {
        type = SEM_INTEGER;
    }

    if (mode==PASS_VAL && TYPE_IS_COMPOSITE(type)) {
        yyerror("arrays, records and set datatypes can only be passed by refference");
    }
    else if (list->param_empty>=MAX_IDF-idf_empty) {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            new_param = (param_t*)malloc(sizeof(param_t));
            new_param->name = idf_table[i]->name;
            new_param->pass_mode = mode;
            new_param->datatype = type;
            list->param[MAX_PARAMS-list->param_empty] = new_param;
            list->param_empty--;
        }
    }
    else {
        printf("too much function parameters\n");
    }
    idf_init();
    return list;
}

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
    if (body->last->return_point==0) {
        sprintf(str_err,"control reaches end of function '%s' without return value",subprogram->func_name);
        yyerror(str_err);
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
        return;
    }

    if (sem_sub->id_is!=ID_FORWARDED_PROC && sem_sub->id_is!=ID_FORWARDED_FUNC) {
        //sanity check, should never reach here
        printf("UNEXPECTED ERROR 21\n");
        exit(EXIT_FAILURE);
    }

    subprogram = sem_sub->subprogram;
    subprogram->label = new_label_subprogram(subprogram->func_name);

    start_new_scope(subprogram);
    configure_stack_size_and_param_lvalues(subprogram);
    declare_formal_parameters(subprogram); //declare them inside the new scope
    new_statement_module(subprogram->label);
}

void subprogram_finit(sem_t *subprogram,statement_t *body) {
    //mark subprogram as well-declared, if needed
    if (subprogram->id_is == ID_FORWARDED_FUNC) {
        subprogram->id_is = ID_FUNC;
        check_for_return_value(subprogram->subprogram,body);
    }
    else if (subprogram->id_is == ID_FORWARDED_PROC) {
        subprogram->id_is = ID_PROC;
    }

    close_current_scope();

    link_statement_to_module_and_return(body);
}

sem_t *declare_function_header(char *id,param_list_t *list,data_t *return_type) {
    sem_t *sem_2;
    var_t *return_value;

    //function name belongs to current scope
    sem_2 = sm_insert(id);
    if (sem_2) {
        sem_2->id_is = ID_FORWARDED_FUNC;
        sem_2->subprogram = (func_t*)malloc(sizeof(func_t));
        sem_2->subprogram->func_name = sem_2->name;
        sem_2->subprogram->stack_size = 0;

        if (!list) {
            yyerror("functions must have at least one parameter.");
            //continue to avoid false error messages
        }

        return_value = (var_t*)malloc(sizeof(var_t));
        return_value->id_is = ID_RETURN;
        return_value->datatype = return_type;
        return_value->name = sem_2->name;
        //do not set the scope or Lvalue here, we are not inside the function yet

        sem_2->subprogram->return_value = return_value;

        configure_formal_parameters(list,sem_2->subprogram);
    }
    else { //sem_2==NULL
        //if the id is used before, an error is printed by sm_insert.
        yyerror("on function declaration");
    }
    return sem_2;
}

sem_t *declare_procedure_header(char *id,param_list_t *list) {
    sem_t *sem_2;

    sem_2  = sm_insert(id);
    if (sem_2) {
        //do something with formal parameters

        sem_2->id_is = ID_FORWARDED_PROC;
        sem_2->subprogram = (func_t*)malloc(sizeof(func_t));
        sem_2->subprogram->func_name = sem_2->name;
        sem_2->subprogram->return_value = NULL;
        sem_2->subprogram->stack_size = 0;

        configure_formal_parameters(list,sem_2->subprogram);
    }
    else { //sem_2==NULL
        //if the id is used before, an error is printed by sm_insert.
        yyerror("on procedure id declaration");
    }
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
