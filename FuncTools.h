#ifndef _FUNC_TOOLS_H
#define _FUNC_TOOLS_H

#define DEFAULT_STR_SIZE 64

typedef struct buf {
    char* buf;
    char* ptr;
    int factor;
    int empty;
} T_buf;

extern T_buf str_buf;
extern T_buf line_buf;

void init_buf(T_buf *buf_ptr);
void bufadd(T_buf *dst_buf, char *src_ptr);

int HexToDec (char* hex);
int BinToDec (char* bin);
int DecToDec (char* dec);

double strFloatToDecFloat(char* string);
double strHexFloatToDecFloat(char* string);
double strBinFloatToDecFloat(char* string);

char isolate_char_const(const char *str);

#endif
