#ifndef _IDENTIFIERS_H_
#define _IDENTIFIERS_H_

#include "semantics.h"

#define MAX_IDF 128

#define IDF_FREE_MEM 1
#define IDF_KEEP_MEM 0

typedef struct idf_t {
    char *name;
    int ival; //used for enum type
} idf_t;

extern idf_t *idf_table;
//extern data_t *idf_data_type;
extern int idf_empty;

void idf_init(int idf_free_memory);
idf_t *idf_find(const char *id);
int idf_insert(char *id);
void idf_set_type(data_t *type);
void idf_addto_record();

#endif
