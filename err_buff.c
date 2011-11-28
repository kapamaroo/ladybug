#include "build_flags.h"
#include "err_buff.h"

int err_num;
FILE *log_file;
char str_err[MAX_ERROR_BUFFER];

void init_err_buff() {
    err_num = 0;

#if LOG_TO_FILE == 1
    log_file = fopen(LOG_FILENAME,"w");
#endif
}
