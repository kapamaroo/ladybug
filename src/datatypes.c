#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //for memcpy()

#include "datatypes.h"
#include "identifiers.h"
#include "mem.h"
#include "symbol_table.h"
#include "expressions.h"
#include "expr_toolbox.h"
#include "err_buff.h"

data_t *usr_datatype; //new user defined datatype
data_t *VIRTUAL_STRING_DATATYPE;

var_t *reference_to_variable_or_enum_element(char *id) {
    sem_t *sem_1;
    var_t *new_enum_const;
    char *lost_id;

    sem_1 = sm_find(id);
    if (sem_1) {
        if (sem_1->id_is==ID_VAR || sem_1->id_is==ID_CONST) {
            return sem_1->var;
        }
        else if (sem_1->id_is==ID_FORWARDED_FUNC) { //we are inside a function declaration
            //the name of the function acts like a variable
            return sem_1->subprogram->return_value;
        }
        else if (sem_1->id_is==ID_TYPEDEF && sem_1->comp->is==TYPE_ENUM) {
            //ALLOW this if only enumerations and subsets can declare an iter_space
            if (strcmp(sem_1->comp->name,id)==0) {
                sprintf(str_err,"'%s' is the name of the enumeration, expected only an element",id);
                yyerror(str_err);
                return lost_var_reference();
            }
            //the ID is an enumeration element
            new_enum_const = (var_t*)malloc(sizeof(var_t));
            new_enum_const->id_is = ID_CONST;
            new_enum_const->datatype = sem_1->comp;
            new_enum_const->name = sem_1->comp->field_name[enum_num_of_id(sem_1->comp,id)];
            new_enum_const->Lvalue = NULL;
            new_enum_const->cond_assign = NULL;
            new_enum_const->ival = enum_num_of_id(sem_1->comp,id);
            //do not set the scope, this is not a variable
            return new_enum_const;
        }
        else {
            sprintf(str_err,"'%s' is not a variable or constant",id);
            yyerror(str_err);
            return lost_var_reference();
        }
    }
    else {
        lost_id = sm_find_lost_symbol(id);
        if (!lost_id) {
            sm_insert_lost_symbol(id);
            sprintf(str_err,"undeclared symbol '%s'",id);
            yyerror(str_err);
        }
        return lost_var_reference();
    }
}

var_t *reference_to_array_element(var_t *v, expr_list_t *list) {
    //reference to array
    expr_t *relative_offset_expr;
    expr_t *final_offset_expr;
    expr_t *cond_expr;
    mem_t *new_mem;
    var_t *new_var;

    if (!list) {
        die("UNEXPECTED_ERROR: null expr_list for array reference");
    }

    if (v) {
        if (v->id_is==ID_VAR) {
            if (v->datatype->is==TYPE_ARRAY) {
                if (valid_expr_list_for_array_reference(v->datatype,list)) {
                    //we print any possible messages in "valid_expr_list_for_array_reference()"
                    relative_offset_expr = make_array_reference(list,v->datatype);
                    cond_expr = make_array_bound_check(list,v->datatype);

                    final_offset_expr = expr_relop_equ_addop_mult(v->Lvalue->offset_expr,OP_PLUS,relative_offset_expr);

                    //we start from the variable's mem position and we add the offset from there
                    new_mem = (mem_t*)malloc(sizeof(mem_t));
                    new_mem = (mem_t*)memcpy(new_mem,v->Lvalue,sizeof(mem_t));

                    new_mem->offset_expr = final_offset_expr;
                    new_mem->size = v->datatype->def_datatype->memsize;

                    new_var = (var_t*)malloc(sizeof(var_t));
                    new_var = (var_t*)memcpy(new_var,v,sizeof(var_t));
                    //new_var->id_is = ID_VAR;
                    new_var->datatype = v->datatype->def_datatype;
                    //new_var->name = v->name;
                    //new_var->scope = v->scope;
                    new_var->Lvalue = new_mem;
                    new_var->cond_assign = cond_expr;
                    return new_var;
                }
            }
            else {
                sprintf(str_err,"variable '%s' is not an array",v->name);
                yyerror(str_err);
            }
        }
        else if (v->id_is==ID_LOST) {
            //avoid duplucate error messages
            return v; //this points to lost variable
        }
        else {
            sprintf(str_err,"id '%s' is not a variable",v->name);
            yyerror(str_err);
        }
        return lost_var_reference();
    }

    die("UNEXPECTED_ERROR: 42");
    return NULL; //keep the compiler happy
}

var_t *reference_to_record_element(var_t *v, char *id) {
    //this is a struct, we take ID's type
    int elem_num;
    var_t *new_var;
    expr_t *offset_expr;
    expr_t *final_expr_offset;
    mem_t *new_mem;

    if (v) {
        if (v->id_is==ID_VAR) {
            if (v->datatype->is==TYPE_RECORD) {
                elem_num = check_for_id_in_datatype(v->datatype,id);
                if (elem_num>=0) {
                    //element's position does not change so it is represented as a hardcoded constant
                    offset_expr = expr_from_hardcoded_int(v->datatype->field_offset[elem_num]);
                    final_expr_offset = expr_relop_equ_addop_mult(v->Lvalue->offset_expr,OP_PLUS,offset_expr);

                    new_mem = (mem_t*)malloc(sizeof(mem_t));
                    new_mem = (mem_t*)memcpy(new_mem,v->Lvalue,sizeof(struct mem_t));
                    new_mem->offset_expr = final_expr_offset;
                    new_mem->size = v->datatype->field_datatype[elem_num]->memsize;

                    new_var = (var_t*)malloc(sizeof(var_t));
                    new_var->id_is = ID_VAR;
                    new_var->datatype = v->datatype->field_datatype[elem_num];
                    //new_var->name = v->datatype->field_name[elem_num]; //BUG strdup the name because we free() it later
                    new_var->name = strdup(v->datatype->field_name[elem_num]);
                    new_var->scope = v->scope;
                    new_var->Lvalue = new_mem;

                    return new_var;
                }
                else {
                    sprintf(str_err,"no element named '%s' in record type",id);
                    yyerror(str_err);
                    return lost_var_reference();
                }
            }
            else {
                yyerror("type of variable is not a record");
                return lost_var_reference();
            }
        }
        else if (v->id_is==ID_LOST) {
            return v; //avoid unreal error messages
        }
        else {
            sprintf(str_err,"id '%s' is not a variable",v->name);
            yyerror(str_err);
            return lost_var_reference();
        }
    }
    else {
        die("INTERNAL_ERROR: 43");
        return NULL; //keep the compiler happy
    }
}

int enum_num_of_id(const data_t *data,const char *id) {
    int i=0;
    //if this function is called, id exists for sure
    while (strcmp(data->field_name[i],id)!=0) {
        i++;
    }
    return data->enum_num[i];
}

data_t *close_datatype_start_new() {
    data_t *old_usr_datatype;

    old_usr_datatype = usr_datatype;
    usr_datatype = (data_t*)malloc(sizeof(data_t));
    return old_usr_datatype;
}

data_t *close_array_type(data_t *def_type) {
    //there is at least one dimension
    int i;
    //    int num_of_elements = 0;
    //
    //    for(i=0;i<usr_datatype->field_num;i++) {
    //      num_of_elements += usr_datatype->dim[i]->range;
    //    }

    //    int n1,n2;

    if (def_type) {
        usr_datatype->def_datatype = def_type;
        //usr_datatype->memsize = num_of_elements * usr_datatype->datadef_type->memsize;

        usr_datatype->memsize = usr_datatype->dim[0]->relative_distance * usr_datatype->dim[0]->range * usr_datatype->def_datatype->memsize;
        return close_datatype_start_new();
    }
    else {
        for(i=0;i<usr_datatype->field_num;i++) {
            free(usr_datatype->dim[i]);
            usr_datatype->dim[i] = NULL;
        }
        return NULL;
    }
}

data_t *close_set_type(data_t *type) {
    //memsize is a power of 2 depending of the elements' data type
    int size=0;
    int bytes;

    usr_datatype->is = TYPE_SET;
    size = calculate_number_of_set_elements(type);
    if (size) {
        usr_datatype->def_datatype = type;
        bytes = size/8;
        if (size%8) {
            bytes++;
        }
        usr_datatype->memsize = bytes;
        return close_datatype_start_new();
    }
    else {
        yyerror("cannot define set type for this data type");
        return NULL;
    }
}

data_t *close_record_type() {
    //memsize of new record data type is already set
    //see idf_addto_record() function

    if (usr_datatype->field_num>0) {
        return close_datatype_start_new();
    }
    else {
        //posibly parse errors
        //record datatypes must have at least one element
        return NULL;
    }
}

/** Memory size could be relative to enum's (subset's) range
 * but we use the same size with integers. we play safe here,
 * for binary compatibility between programs (database
 * programs for example). Some programs could use integers
 * and some could use enums or subsets for the same task.
 * This goes to far, but whatever :)
 */
data_t *close_enum_type() {
    int i;
    char *id;
    for (i=0;i<MAX_IDF-idf_empty;i++) {
        id = idf_table[i].name;
        if (sm_find(id)) {
            sprintf(str_err,"Identifier `%s` already exists in this scope",id);
            yyerror(str_err);
            usr_datatype->memsize = MEM_SIZEOF_INT;
            return close_datatype_start_new();
        }
    }

    usr_datatype->is = TYPE_ENUM;
    usr_datatype->def_datatype = SEM_INTEGER;
    if ((MAX_IDF - idf_empty)<=MAX_FIELDS) {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            usr_datatype->field_name[i] = idf_table[i].name;
            usr_datatype->enum_num[i] = idf_table[i].ival;
            usr_datatype->field_num++;
        }
        idf_init(IDF_KEEP_MEM);
    }
    else {
        yyerror("Too much identifiers in enum type");
    }

    usr_datatype->memsize = MEM_SIZEOF_INT;
    return close_datatype_start_new();
}

data_t *close_subset_type(expr_t *l1, expr_t *l2) {
    //this function defines a new type which is a subset of another
    //if everything ok set the *record pointer
    int i;

    if (!l1 || !l2) {
        die("INTERNAL ERROR: null expression in subset definition");
    }

    if (!TYPE_IS_SUBSET_VALID(l1->datatype) || !TYPE_IS_SUBSET_VALID(l2->datatype)) {
        yyerror("invalid datatype in subset declaration");
        return NULL;
    }

    if (l1->datatype!=l2->datatype) {
        yyerror("in subset definition, bounds have different data types");
        return NULL;
    }

    if (l1->expr_is!=EXPR_HARDCODED_CONST || l2->expr_is!=EXPR_HARDCODED_CONST) {
        yyerror("subset limits MUST be constants");
        return NULL;
    }

    //this also works in the case of char limits
    if (l2->ival - l1->ival + 1<=0 || l2->ival - l1->ival + 1>MAX_FIELDS) {
        yyerror("subset limits are incorrect");
        return NULL;
    }

    //type of expressions is either SEM_CHAR or SEM_INTEGER or an enumeration
    usr_datatype->is = TYPE_SUBSET;
    usr_datatype->def_datatype = l1->datatype;

    usr_datatype->field_num = l2->ival - l1->ival +1;
    for (i=0;i<usr_datatype->field_num;i++) {
        usr_datatype->enum_num[i] = l1->ival + i;
    }
    usr_datatype->memsize = l1->datatype->memsize;
    return close_datatype_start_new();
}

void add_dim_to_array_type(dim_t *dim) {
    int i;

    usr_datatype->is = TYPE_ARRAY;

    //this is the last dimension so far
    dim->relative_distance = 1;

    if (usr_datatype->field_num<MAX_ARRAY_DIMS) {
        //update dim_size in memory
        for (i=usr_datatype->field_num-1;i>=0;i--) {
            usr_datatype->dim[i]->relative_distance *= dim->range;
        }
        usr_datatype->dim[usr_datatype->field_num] = dim;
        usr_datatype->field_num++;
    }
    else {
        //yyerror("too much dimensions");
    }
}

void make_type_definition(char *id, data_t *type) {
    sem_t *sem_1;
    if (type!=NULL) {
        if (!sm_find(id)) {
            sem_1 = sm_insert(id);
            //set semantics to symbol
            sem_1->id_is = ID_TYPEDEF;
            sem_1->comp = type;
            sem_1->comp->name = sem_1->name;
        }
        else {
            yyerror("id type declaration already exists");
        }
    }
    else {
        yyerror("data type NOT defined (debugging info)");
    }
}

int calculate_number_of_set_elements(data_t *type) {
    //returns the size of the new SET type, or zero if
    //we cannot create it
    if (!type) {
        return 0;
    }
    else if (type->is==TYPE_CHAR) {
        return MAX_SET_ELEM;
    }
    else if (type->is==TYPE_BOOLEAN) {
#warning what is the point of a boolean set??
        return 2; //number of elements
    }
    else if (type->is==TYPE_ENUM || type->is==TYPE_SUBSET) {
        if (type->field_num <= MAX_SET_ELEM) {
            return type->field_num;
        }
    }
    return 0;
}

int check_for_id_in_datatype(data_t *datatype,const char *id) {
    //checks if id exists in the given composite type and returns its index
    int i;
    for (i=0;i<datatype->field_num;i++) {
        if (!datatype->field_name[i]) {
            return -1;
        }
        else if (strcmp(datatype->field_name[i],id)==0) {
            return i;
        }
    }
    return -1;
}
