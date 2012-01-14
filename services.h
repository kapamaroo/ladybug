#ifndef _SERVICES_H
#define _SERVICES_H

//we implement single float numbers

typedef enum mips_service_t {
    SVC_PRINT_INT=1,
    SVC_PRINT_REAL=2,
    SVC_PRINT_STRING=4,
    SVC_READ_INT=5,
    SVC_READ_REAL=6,
    SVC_READ_STRING=8,
    SVC_EXIT=10,
    SVC_PRINT_CHAR=11,
    SVC_READ_CHAR=12,
} mips_service_t;

#endif
