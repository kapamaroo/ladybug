#include <stdio.h>
#include <stdlib.h>

#include "build_flags.h"
#include "subprograms.h"
#include "symbol_table.h"
#include "mem_reg.h"
#include "err_buff.h"

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
        yyerror("ERROR: arrays, records and set datatypes can only be passed by refference");
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

void subprogram_init(sem_t *sem_sub) {
    //DO NOT OPEN A NEW SCOPE HERE!!
    //put the formal parameters of subprogram in the symbol table
    //the scope of this subprogram is already openned after the
    //sub_header declaration.
    //also DO NOT CLOSE the subprogram's scope here

    if (!sem_sub) {
        //name of subprogram didn't inserted to symbol table
        //an error message is printed by sm_insert()
        //we must return. we are going to get lot's of unreal
        //error messages afterwards, ...abord compiling
        //FIXME
        return;
    }
    if (sem_sub->id_is==ID_PROC || sem_sub->id_is==ID_FUNC) {
        //Multiple body definitions for a subprogram are forbidden
        //for more information see the comments in semantics.h
        yyerror("ERROR: Multiple body definition for subprogram.");
        return;
    }

    if (sem_sub->id_is!=ID_FORWARDED_PROC && sem_sub->id_is!=ID_FORWARDED_FUNC) {
        //sanity check, should never reach here
        printf("UNEXPECTED ERROR 21\n");
        exit(EXIT_FAILURE);
    }

    //if (sem_sub->id_is)

    declare_formal_parameters(sem_sub->subprogram);
    new_module(sem_sub->subprogram);
}

void subprogram_finit(sem_t *subprogram,ir_node_t *body) {
    ir_node_t *ir_return;

    //mark subprogram as well-declared, if needed
    if (subprogram->id_is == ID_FORWARDED_FUNC) {
        subprogram->id_is = ID_FUNC;
        check_for_return_value(subprogram->subprogram,body);
    }
    else if (subprogram->id_is == ID_FORWARDED_PROC) {
        subprogram->id_is = ID_PROC;
        ir_return = new_ir_node_t(NODE_RETURN_PROC);
        body = link_stmt_to_stmt(ir_return,body);
    }

    close_current_scope(); //now close the scope of subprogram

    link_stmt_to_tree(body);
    return_to_previous_module();
}

sem_t *declare_function_header(char *id,param_list_t *list,data_t *return_type) {
    sem_t *sem_2;
    var_t *return_value;

    //function name belongs to current scope
    sem_2 = sm_insert(id);
    //open new scope after the declaration of the function and before the creation of the return_value variable (to get the new scope)
    if (sem_2) {
        sem_2->id_is = ID_FORWARDED_FUNC;
        sem_2->subprogram = (func_t*)malloc(sizeof(func_t));
        sem_2->subprogram->func_name = sem_2->name;
        sem_2->subprogram->return_datatype = return_type;

        if (!list) {
            yyerror("ERROR: functions must have at least one parameter.");
            //continue to avoid false error messages
        }

        configure_formal_parameters(list,sem_2->subprogram);
        configure_stack_size_and_param_lvalues(sem_2->subprogram);
        start_new_scope(SCOPE_FUNC,sem_2->subprogram);

        //only for functions
        //use the `var` element from the `sem_t` struct to reffer to the return value of the function
        return_value = (var_t*)malloc(sizeof(var_t));
        return_value->id_is = ID_RETURN;
        return_value->datatype = return_type;
        return_value->name = sem_2->name;
        return_value->scope = get_current_scope();
        return_value->Lvalue = return_from_stack_lvalue(sem_2->subprogram);
        sem_2->var = return_value;

    }
    else { //sem_2==NULL
        //if the id is used before, an error is printed by sm_insert.
        yyerror("ERROR: on function declaration");
    }
    return sem_2;
}

sem_t *declare_procedure_header(char *id,param_list_t *list) {
    sem_t *sem_2;

    sem_2  = sm_insert(id);
    //open new scope after the declaration of the procedure
    if (sem_2) {
        //do something with formal parameters

        sem_2->id_is = ID_FORWARDED_PROC;
        sem_2->subprogram = (func_t*)malloc(sizeof(func_t));
        sem_2->subprogram->func_name = sem_2->name;
        sem_2->subprogram->return_datatype = void_datatype; //we could have it NULL but this is more generall

        configure_formal_parameters(list,sem_2->subprogram);
        configure_stack_size_and_param_lvalues(sem_2->subprogram);
        start_new_scope(SCOPE_PROC,sem_2->subprogram);
    }
    else { //sem_2==NULL
        //if the id is used before, an error is printed by sm_insert.
        yyerror("ERROR: on procedure id declaration");
    }
    return sem_2;
}

void forward_subprogram_declaration(sem_t *subprogram) {
    if (subprogram) {
        close_current_scope();
        if (subprogram->id_is!=ID_FORWARDED_FUNC) {
            yyerror("ERROR: only functions can be forwarded");
        }
    }
    else {
        //either invalid forward on non function id, or more than one forwardes in a row
        yyerror("ERROR: invalid function name or multiple forwards in a row");
    }
}
