#include <stdio.h>
#include <stdlib.h>

#include "build_flags.h"
#include "subprograms_toolbox.h"
#include "identifiers.h"
#include "datatypes.h"
#include "err_buff.h"

param_list_t *param_insert(param_list_t *new_list,pass_t mode,data_t *type) {
    int i;
    param_t *new_param;
    param_list_t *list;

    if (!new_list) {
        list = (param_list_t*)calloc(1,sizeof(param_list_t));
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
        yyerror("arrays, records and set datatypes can only be passed by reference");
    }
    else if (list->param_empty>=MAX_IDF-idf_empty) {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            new_param = (param_t*)calloc(1,sizeof(param_t));
            new_param->name = idf_table[i].name;
            new_param->pass_mode = mode;
            new_param->datatype = type;
            list->param[MAX_PARAMS-list->param_empty] = new_param;
            list->param_empty--;
        }
    }
    else {
        printf("too much function parameters\n");
    }
    idf_init(IDF_KEEP_MEM);
    return list;
}

sem_t *reference_to_forwarded_function(char *id) {
    //this function was declared before in this scope and now we define it, stack is already configured
    sem_t *sem_2;

    sem_2 = sm_find(id);
    if (sem_2) {
        if (sem_2->id_is==ID_FUNC) {
            yyerror("Multiple body definitions for a function");
        }
        else if (sem_2->id_is==ID_PROC) {
            yyerror("id is a procedure not a function and is already declared");
        }
        else if (sem_2->id_is==ID_FORWARDED_PROC) {
            yyerror("id is a procedure not a function, and procedures cannot be forwarded");
        }
        else if (sem_2->id_is!=ID_FORWARDED_FUNC) {
            yyerror("id is not declared as a subprogram");
        }

        free(id); //flex strdup'ed it
        return sem_2;
    }
    else {
        sprintf(str_err,"invalid forwarded function. '%s' not declared before in this scope",id);
        sm_insert_lost_symbol(id,str_err);
    }
    return NULL;
}

func_t *find_subprogram(char *id) {
    sem_t *sem_1;

    sem_1 = sm_find(id);
    if (sem_1) {
        free(id); //flex strdup'ed it

        //it is possible to call a subprogram before defining its body,
        //so check for _FORWARDED_ subprograms too
        //if the sub_type is valid, continue as the subprogram args are correct,
        //to avoid false error messages afterwards

        if (sem_1->id_is == ID_FUNC || sem_1->id_is == ID_FORWARDED_FUNC) {
            sprintf(str_err,"invalid '%s' function call, expected procedure",sem_1->name);
            yyerror(str_err);
        } else if (sem_1->id_is!=ID_PROC && sem_1->id_is!=ID_FORWARDED_PROC) {
            yyerror("ID is not a subprogram.");
        } else {
            return sem_1->subprogram;
        }
    } else {
        sprintf(str_err,"undeclared subprogram '%s'",id);
        sm_insert_lost_symbol(id,str_err);
    }
    return NULL;
}
