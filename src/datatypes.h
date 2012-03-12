#ifndef _DATATYPES_H_
#define _DATATYPES_H_

#include "semantics.h"

//pointers to standard type structures
extern data_t *SEM_INTEGER;
extern data_t *SEM_REAL;
extern data_t *SEM_BOOLEAN;
extern data_t *SEM_CHAR;

extern data_t *usr_datatype; //new user defined datatype
extern data_t *VIRTUAL_STRING_DATATYPE;
extern data_t *void_datatype; //datatype of lost symbols

void init_datatypes();

data_t *close_datatype_start_new();
data_t *close_array_type(data_t *def_type);
data_t *close_set_type(data_t *type);
data_t *close_record_type();
data_t *close_enum_type();
data_t *close_subset_type(expr_t *l1, expr_t *l2);

void add_dim_to_array_type(dim_t *dim);

int calculate_number_of_set_elements(data_t *type);
int check_for_id_in_datatype(data_t *datatype,const char *id);

data_t *reference_to_typename(char *id);
var_t *reference_to_variable_or_enum_element(char *id);
var_t *reference_to_array_element(var_t *v, expr_list_t *list);
var_t *reference_to_record_element(var_t *v, char *id);

int enum_num_of_id(const data_t *data,const char *id);

#endif
