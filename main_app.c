#include <stdio.h>
#include "build_flags.h"

#if LEX_MAIN == 0

void help() {
    printf("%s version %d.%d\n",PROGRAM_NAME,PROGRAM_MAJOR_VERSION,PROGRAM_MINOR_VERSION);
    printf("Usage: The program takes one parameter, the source code file.\nexample: ladybug src.p\n");
}
#endif
