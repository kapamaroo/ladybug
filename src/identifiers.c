#include <stdio.h>
#include <stdlib.h>

#include "identifiers.h"
#include "datatypes.h"
#include "err_buff.h"

idf_t *idf_table;
data_t *idf_data_type;
int idf_empty = MAX_IDF;

idf_t *idf_find(const char *id) {
    int i;
    for (i=0;i<MAX_IDF;i++) {
        if (idf_table[i].name) {
            if (strcmp(id,idf_table[i].name)==0) {
                return &idf_table[i];
            }
        }
        else {
            break;
        }
    }
    return NULL;
}

int idf_insert(char *id) {
    //every identifier's id must be unique in the scope
    idf_t *new_idf;
    if (!idf_empty) {
        die("INTERNAL ERROR: identifier table is full, cannot insert new id's.");
        return 0; //keep the compiler happy
    }
    else if (idf_find(id)) {
        sprintf(str_err,"identifier `%s` already exists",id);
        yyerror(str_err);
        return 0;
    }
    else {
        new_idf = &idf_table[MAX_IDF-idf_empty];
        new_idf->name = id; //strdup(id); //do not strdup, bison did
        new_idf->ival = MAX_IDF-idf_empty;
        idf_empty--;
        return 1;
    }
}

void idf_set_type(data_t *type) {
    if (type) {
        idf_data_type = type;
    }
    else {
        idf_init(IDF_KEEP_MEM);
        yyerror("data type for record field is not defined");
    }
}

void idf_addto_record() {
    int i;

    usr_datatype->is = TYPE_RECORD;

    if ((usr_datatype->field_num + MAX_IDF - idf_empty)<=MAX_FIELDS) {
        for (i=0;i<MAX_IDF-idf_empty;i++) {
            //if no identifiers, we don't reach here
            if (check_for_id_in_datatype(usr_datatype,idf_table[i].name)<0) {
                usr_datatype->field_name[usr_datatype->field_num] = idf_table[i].name;
                usr_datatype->field_datatype[usr_datatype->field_num] = idf_data_type;
                usr_datatype->field_offset[usr_datatype->field_num] = usr_datatype->memsize; //memsize so far
                usr_datatype->field_num++;
                usr_datatype->memsize += idf_data_type->memsize;
            }
            else {
                sprintf(str_err,"'%s' declared previously in '%s' record type, ignoring",idf_table[i].name,usr_datatype->name);
                yyerror(str_err);
            }
        }
    }
    else {
        yyerror("Too much fields in record type");
    }
    idf_init(IDF_KEEP_MEM);
}

void idf_init(int idf_free_memory) {
    static unsigned int first_call = 1;

    if (first_call) {
        idf_table = (idf_t*)calloc(MAX_IDF,sizeof(idf_t));
        first_call = 0;
    }

    int i;
    idf_data_type = NULL;
    for (i=0;i<MAX_IDF-idf_empty;i++) {
        if (idf_free_memory) {
            free(idf_table[i].name);
        }
        //free(idf_table[i]);
        idf_table[i].name = NULL;
    }
    idf_empty = MAX_IDF;
}
