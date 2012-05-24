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

data_t *SEM_INTEGER;
data_t *SEM_REAL;
data_t *SEM_BOOLEAN;
data_t *SEM_CHAR;

data_t *usr_datatype; //new user defined datatype
data_t *VIRTUAL_STRING_DATATYPE;
data_t *void_datatype; //datatype of lost symbols

void init_datatypes() {
    sem_t *sem_INTEGER;
    sem_t *sem_REAL;
    sem_t *sem_BOOLEAN;
    sem_t *sem_CHAR;

    //insert the standard types
    sem_INTEGER = sm_insert("integer",ID_TYPEDEF);
    sem_INTEGER->comp = (data_t*)calloc(1,sizeof(data_t));
    sem_INTEGER->comp->is = TYPE_INT;
    sem_INTEGER->comp->def_datatype = sem_INTEGER->comp;
    sem_INTEGER->comp->name = sem_INTEGER->name;
    sem_INTEGER->comp->memsize = MEM_SIZEOF_INT;
    SEM_INTEGER = sem_INTEGER->comp;

    sem_REAL = sm_insert("real",ID_TYPEDEF);
    sem_REAL->comp = (data_t*)calloc(1,sizeof(data_t));
    sem_REAL->comp->is = TYPE_REAL;
    sem_REAL->comp->def_datatype = sem_REAL->comp;
    sem_REAL->comp->name = sem_REAL->name;
    sem_REAL->comp->memsize = MEM_SIZEOF_REAL;
    SEM_REAL = sem_REAL->comp;

    sem_BOOLEAN = sm_insert("boolean",ID_TYPEDEF);
    sem_BOOLEAN->comp = (data_t*)calloc(1,sizeof(data_t));
    sem_BOOLEAN->comp->is = TYPE_BOOLEAN;
    sem_BOOLEAN->comp->def_datatype = sem_BOOLEAN->comp;
    sem_BOOLEAN->comp->name = sem_BOOLEAN->name;
    sem_BOOLEAN->comp->memsize = MEM_SIZEOF_BOOLEAN;
    SEM_BOOLEAN = sem_BOOLEAN->comp;

    sem_CHAR = sm_insert("char",ID_TYPEDEF);
    sem_CHAR->comp = (data_t*)calloc(1,sizeof(data_t));
    sem_CHAR->comp->is = TYPE_CHAR;
    sem_CHAR->comp->def_datatype = sem_CHAR->comp;
    sem_CHAR->comp->name = sem_CHAR->name;
    sem_CHAR->comp->memsize = MEM_SIZEOF_CHAR;
    SEM_CHAR = sem_CHAR->comp;

    usr_datatype = (data_t*)calloc(1,sizeof(data_t));

    void_datatype = (data_t*)calloc(1,sizeof(struct data_t));
    void_datatype->is = TYPE_VOID;
    void_datatype->def_datatype = void_datatype;
    void_datatype->name = "__void_datatype__";
    void_datatype->memsize = 0;

    //(d->is==TYPE_ARRAY && d->field_num==1 && d->def_datatype->is==TYPE_CHAR)
    VIRTUAL_STRING_DATATYPE = (data_t*)calloc(1,sizeof(data_t));
    VIRTUAL_STRING_DATATYPE->is = TYPE_ARRAY;
    VIRTUAL_STRING_DATATYPE->field_num = 1;
    VIRTUAL_STRING_DATATYPE->def_datatype = SEM_CHAR;
}

var_t *reference_to_variable_or_enum_element(char *id) {
    sem_t *sem_1;
    var_t *new_enum_const;
    char *lost_id;

    sem_1 = sm_find(id);
    if (!sem_1) {
        sprintf(str_err,"bad reference: undeclared symbol '%s'",id);
        sm_insert_lost_symbol(id,str_err);
        return lost_var_reference();
    }

    if (sem_1->id_is==ID_VAR || sem_1->id_is==ID_CONST) {
        free(id); //flex strdup'ed it
        return sem_1->var;
    }

    if (sem_1->id_is==ID_FORWARDED_FUNC) { //we are inside a function declaration
        //the name of the function acts like a variable
        free(id); //flex strdup'ed it
        return sem_1->subprogram->return_value;
    }

    if (sem_1->id_is!=ID_TYPEDEF || sem_1->comp->is!=TYPE_ENUM) {
        sprintf(str_err,"'%s' is not a variable or constant",id);
        yyerror(str_err);
        free(id); //flex strdup'ed it
        return lost_var_reference();
    }

    //ALLOW this if only enumerations and subsets can declare an iter_space
    if (strcmp(sem_1->comp->name,id)==0) {
        sprintf(str_err,"'%s' is the name of the enumeration, expected only an element",id);
        yyerror(str_err);
        free(id); //flex strdup'ed it
        return lost_var_reference();
    }

    //ID is an enumeration element

    new_enum_const = (var_t*)calloc(1,sizeof(var_t));
    new_enum_const->id_is = ID_CONST;
    new_enum_const->datatype = sem_1->comp;
    new_enum_const->name = sem_1->comp->field_name[enum_num_of_id(sem_1->comp,id)];
    new_enum_const->Lvalue = NULL;
    new_enum_const->cond_assign = NULL;
    new_enum_const->ival = enum_num_of_id(sem_1->comp,id);

    new_enum_const->to_expr = expr_version_of_variable(new_enum_const);

    new_enum_const->status_value = VALUE_VALID;
#warning do we care about unused constants?
    new_enum_const->status_use = USE_NONE;
    new_enum_const->status_known = KNOWN_YES;

    //do not set the scope, this is not a variable but a constant value
    free(id); //flex strdup'ed it
    return new_enum_const;
}

var_t *reference_to_array_element(var_t *v, expr_list_t *list) {
    //reference to array
    expr_t *relative_offset_expr;
    expr_t *final_offset_expr;
    expr_t *cond_expr;
    mem_t *new_mem;
    var_t *new_var;

    if (!v || !list) {
        die("UNEXPECTED_ERROR: bad parameters for array reference");
    }

    if (v->id_is!=ID_VAR) {
        return lost_var_reference();
    }

    if (v->datatype->is!=TYPE_ARRAY) {
        sprintf(str_err,"variable '%s' is not an array",v->name);
        yyerror(str_err);
        return lost_var_reference();
    }

    if (!valid_expr_list_for_array_reference(v->datatype,list)) {
        return lost_var_reference();
    }

    //we print any possible messages in "valid_expr_list_for_array_reference()"

    new_var = (var_t*)calloc(1,sizeof(var_t));
    new_var = (var_t*)memcpy(new_var,v,sizeof(var_t));

    //do not inherit base Lvalue, we calculate the final Lvalue later
    new_var->Lvalue = NULL;

    new_var->datatype = v->datatype->def_datatype;
    new_var->cond_assign = cond_expr;

    new_var->to_expr = expr_version_of_variable(new_var);

    new_var->from_comp = (info_comp_t*)calloc(1,sizeof(info_comp_t));
    new_var->from_comp->comp_type = v->datatype->is;
    new_var->from_comp->array.base = v;
    new_var->from_comp->array.index = list;

    return new_var;
}

var_t *reference_to_record_element(var_t *v, char *id) {
    //this is a struct, we take ID's type
    int elem_num;
    var_t *new_var;
    expr_t *offset_expr;
    expr_t *final_expr_offset;
    mem_t *new_mem;

    if (!v) {
        die("INTERNAL_ERROR: 43");
        return NULL; //keep the compiler happy
    }

    if (v->id_is!=ID_VAR) {
        free(id); //flex strdup'ed it
        return lost_var_reference();
    }

    if (v->datatype->is!=TYPE_RECORD) {
        yyerror("type of variable is not a record");
        free(id); //flex strdup'ed it
        return lost_var_reference();
    }

    elem_num = check_for_id_in_datatype(v->datatype,id);
    if (elem_num<0) {
        sprintf(str_err,"no element named '%s' in record type",id);
        yyerror(str_err);
        free(id); //flex strdup'ed it
        return lost_var_reference();
    }

    //element's position does not change so it is represented as a hardcoded constant

    //offset_expr = expr_from_hardcoded_int(v->datatype->field_offset[elem_num]);
    //size = v->datatype->field_datatype[elem_num]->memsize;

    new_var = (var_t*)calloc(1,sizeof(var_t));
    new_var = (var_t*)memcpy(new_var,v,sizeof(var_t));

    //do not inherit base Lvalue, we calculate the final Lvalue later
    new_var->Lvalue = NULL;

    new_var->name = v->datatype->field_name[elem_num]; //BUG strdup the name because we free() it later
    //new_var->name = strdup(v->datatype->field_name[elem_num]);

    new_var->datatype = v->datatype->field_datatype[elem_num];

    new_var->to_expr = expr_version_of_variable(new_var);

    new_var->from_comp = (info_comp_t*)calloc(1,sizeof(info_comp_t));
    new_var->from_comp->comp_type = v->datatype->is;
    new_var->from_comp->record.base = v;
    new_var->from_comp->record.el_offset = expr_from_hardcoded_int(elem_num);

    free(id); //flex strdup'ed it
    return new_var;
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
    usr_datatype = (data_t*)calloc(1,sizeof(data_t));
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
    if (type) {
        sem_1 = sm_find(id);
        if (!sem_1) {
            sem_1 = sm_insert(id,ID_TYPEDEF);
            //set semantics to symbol
            sem_1->comp = type;
            sem_1->comp->name = sem_1->name;
        }
        else {
            free(id); //flex strdup'ed it
            yyerror("id type declaration already exists");
        }
    }
    else {
        free(id); //flex strdup'ed it
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
