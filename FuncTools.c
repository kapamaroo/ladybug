
/*****************************************************created by kostadinos parasyris ******************************************************************/
/* 		This file has functions that convert strings that represent float or integer numbers into float or integer numbers		       */
/*		Its function has a pointer to a string as a parameter and it returns a float or integer value					       */
/*******************************************************************************************************************************************************/
#include "FuncTools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"

T_buf str_buf;
T_buf line_buf;

void init_buf(T_buf *buf_ptr) {
    buf_ptr->factor = 1;
    buf_ptr->empty = DEFAULT_STR_SIZE;
    free(buf_ptr->buf); //any previous string
    buf_ptr->buf = (char*)malloc(DEFAULT_STR_SIZE*sizeof(char));
    buf_ptr->ptr = buf_ptr->buf;
}

void bufadd(T_buf *dst_buf, char *src_ptr) {
    while (*src_ptr) {
        while (dst_buf->empty && *src_ptr) {
            *dst_buf->ptr++ = *src_ptr++;
            dst_buf->empty--;
        }
        if (*src_ptr) {
            //source bigger than DEFAULT_STR_SIZE, need more memory
            dst_buf->factor++;
            dst_buf->buf = (char*)realloc((void*)dst_buf->buf,dst_buf->factor*DEFAULT_STR_SIZE*sizeof(char));
            dst_buf->empty = DEFAULT_STR_SIZE;
            dst_buf->ptr = &dst_buf->buf[(DEFAULT_STR_SIZE-1)*(dst_buf->factor-1)+1];
        }
    }
}

/********************************************************************/
/*		function HexToDec				    */
/*Takes a pointer to a string and returns its demical represatation */
/*the string must represent a hex integer number		    */
/*te function returns an integer				    */
/********************************************************************/

int HexToDec (char* hex) {
    int i,j;
    int value = 0;
    int power=1;
    int length = (int) strlen(hex);
    for (i = 0; i < length ; i++) {
        power = 1;
        for (j = 1; j<length-i;j++) {
            power = power*16;
        }
        switch (hex[i]) {
        case '0':
            break;
        case '1':
            value= value+1*power;
            break;
        case '2':
            value= value+2*power;
            break;
        case '3':
            value= value+3*power;
            break;
        case '4':
            value= value+4*power;
            break;
        case '5':
            value= value+5*power;
            break;
        case '6':
            value= value+6*power;
            break;
        case '7':
            value= value+7*power;
            break;
        case '8':
            value= value+8*power;
            break;
        case '9':
            value= value+9*power;
            break;
        case 'a':
        case 'A':
            value= value+10*power;
            break;
        case 'b':
        case 'B':
            value= value+11*power;
            break;
        case 'c':
        case 'C':
            value= value+12*power;
            break;
        case 'd':
        case 'D':
            value= value+13*power;
            break;
        case 'e':
        case 'E':
            value= value+14*power;
            break;
        case 'f':
        case 'F':
            value= value+15*power;
            break;
        }
    }
    return(value);
}

/********************************************************************/
/*		function BinToDec				    */
/*Takes a pointer to a string and returns its demical represatation */
/*the string must represent a binary integer number		    */
/*te function returns an integer				    */
/********************************************************************/
int BinToDec (char* bin) {
    int i,j;
    int value = 0;
    int power=1;
    int length = (int) strlen(bin);
    for (i = 0; i < length ; i++) {
        power = 1;
        for (j = 1; j<length-i;j++) {
            power = power*2;
        }
        switch (bin[i]) {
        case '0':
            break;
        case '1':
            value= value+1*power;
            break;
        }
    }
    return(value);
}


/********************************************************************/
/*		function DecToDec				    */
/*Takes a pointer to a string and returns its demical represatation */
/*the string must represent a demical integer number		    */
/*te function returns an integer				    */
/********************************************************************/
int DecToDec (char* dec) {
    int i,j;
    int value = 0;
    int power=1;
    int length = (int) strlen(dec);
    for (i = 0; i < length ; i++) {
        power = 1;
        for (j = 1; j<length-i;j++) {
            power = power*10;
        }
        switch (dec[i]) {
        case '0':
            break;
        case '1':
            value= value+1*power;
            break;
        case '2':
            value= value+2*power;
            break;
        case '3':
            value= value+3*power;
            break;
        case '4':
            value= value+4*power;
            break;
        case '5':
            value= value+5*power;
            break;
        case '6':
            value= value+6*power;
            break;
        case '7':
            value= value+7*power;
            break;
        case '8':
            value= value+8*power;
            break;
        case '9':
            value= value+9*power;
            break;
        }
    }
    return(value);
}

/********************************************************************/
/*		function strFloatToDecFloat			    */
/*Takes a pointer to a string and returns its demical represatation */
/*the string must represent a demical float number		    */
/*te function returns a double number				    */
/********************************************************************/
double strFloatToDecFloat(char* string) {

    int i,k,dec;
    float value = 0;
    float power=1;
    int length = (int) strlen(string);
    for (i = 0; i < length && string[i]!='e' && string[i]!='E' && string[i]!='.'; i++) {
        value= value*10;
        switch (string[i]) {
        case '0':
            break;
        case '1':
            value= value+1;
            break;
        case '2':
            value= value+2;
            break;
        case '3':
            value= value+3;
            break;
        case '4':
            value= value+4;
            break;
        case '5':
            value= value+5;
            break;
        case '6':
            value= value+6;
            break;
        case '7':
            value= value+7;
            break;
        case '8':
            value= value+8;
            break;
        case '9':
            value= value+9;
            break;
        }
    }
    if ( string[i] == '.') {
        k= i + 1;
        power = 1;
        for (i = k; i < length && string[i]!='e' && string[i]!='E' ; i++) {
            power = power/10;
            switch (string[i]) {
            case '0':
                break;
            case '1':
                value= value+1*power;
                break;
            case '2':
                value= value+2*power;
                break;
            case '3':
                value= value+3*power;
                break;
            case '4':
                value= value+4*power;
                break;
            case '5':
                value= value+5*power;
                break;
            case '6':
                value= value+6*power;
                break;
            case '7':
                value= value+7*power;
                break;
            case '8':
                value= value+8*power;
                break;
            case '9':
                value= value+9*power;
                break;
            }
        }
    }
    if (i < length && (string[i] == 'e'||string[i] == 'E')) {
        if (string[i+1] == '+') {
            dec = DecToDec((string+i+2));
            for (i = 0; i < dec; i++) {
                value = value*10;
            }
        }
        if (string[i+1] == '-') {
            dec = DecToDec((string+i+2));
            for (i = 0; i < dec; i++) {
                value = value/10;
            }
        }
        else {
            dec = DecToDec((string+i+1));
            for (i = 0; i < dec; i++) {
                value = value*10;
            }
        }
    }
    return(value);
}

/********************************************************************/
/*		function HexToDec				    */
/*Takes a pointer to a string and returns its demical represatation */
/*the string must represent a hex float number		    	    */
/*te function returns a double number				    */
/********************************************************************/


double strHexFloatToDecFloat(char* string) {
    int i;
    double value = 0;
    float power = 1;

    int length = (int) strlen(string);
    for (i = 0; i < length && string[i]!='.'; i++) {
        value= value*16;
        switch (string[i]) {
        case '0':
            break;
        case '1':
            value= value+1;
            break;
        case '2':
            value= value+2;
            break;
        case '3':
            value= value+3;
            break;
        case '4':
            value= value+4;
            break;
        case '5':
            value= value+5;
            break;
        case '6':
            value= value+6;
            break;
        case '7':
            value= value+7;
            break;
        case '8':
            value= value+8;
            break;
        case '9':
            value= value+9;
            break;
        case 'a':
        case 'A':
            value= value+10;
            break;
        case 'b':
        case 'B':
            value= value+11;
            break;
        case 'c':
        case 'C':
            value= value+12;
            break;
        case 'd':
        case 'D':
            value= value+13;
            break;
        case 'e':
        case 'E':
            value= value+14;
            break;
        case 'f':
        case 'F':
            value= value+15;
            break;
        }
    }
    for (i = i+1; i < length; i++) {
        power = power/16.0f;
        switch (string[i]) {
        case '0':
            break;
        case '1':
            value= value+( double )1*power;
            break;
        case '2':
            value= value+( double )2*power;
            break;
        case '3':
            value= value+( double )3*power;
            break;
        case '4':
            value= value+( double )4*power;
            break;
        case '5':
            value= value+( double )5*power;
            break;
        case '6':
            value= value+( double )6*power;
            break;
        case '7':
            value= value+( double )7*power;
            break;
        case '8':
            value= value+( double )8*power;
            break;
        case '9':
            value= value+( double )9*power;
            break;
        case 'a':
        case 'A':
            value= value+( double )10*power;
            break;
        case 'b':
        case 'B':
            value= value+( double )11*power;
            break;
        case 'c':
        case 'C':
            value= value+( double )12*power;
            break;
        case 'd':
        case 'D':
            value= value+( double )13*power;
            break;
        case 'e':
        case 'E':
            value= value+( double )14*power;
            break;
        case 'f':
        case 'F':
            value= value+( double )15*power;
            break;
        }
    }
    return(value);
}

/********************************************************************/
/*		function strBinFloatToDecFloat			    */
/*Takes a pointer to a string and returns its demical represatation */
/*the string must represent a binary float number		    */
/*te function returns a double number				    */
/********************************************************************/

double strBinFloatToDecFloat(char* string) {
    int i;
    double value = 0;
    float power = 1;
    int length = (int) strlen(string);
    for (i = 0; i < length && string[i]!='.'; i++) {
        value= value*2;
        switch (string[i]) {
        case '0':
            break;
        case '1':
            value= value+1;
            break;
        }
    }
    for (i = i+1; i < length; i++) {
        power = power/2.0f;
        switch (string[i]) {
        case '0':
            break;
        case '1':
            value= value+( double )1*power;
            break;
        }
    }
    return(value);
}

/**this function is called only from the lex analyser to isolate the const char value from the ''' of yytext */
char isolate_char_const(const char *str) {
    int len;
    char cconst;

    len = strlen(str);
    if (len==2) { //empty CCONST
        //the empty char const is not allowed at the moment but maybe it will be added
        //the lexer recognizes only char consts with a value (len==3 or len==4 for magic chars)
        printf("_INTERNAL_ERROR: empty char const is not allowed\n");
        cconst = ' '; //the space char, NULL is not printable
    }
    else if (len==3) { //simple CCONST
        cconst = str[1];
    }
    else if (len==4) { //magic char for CCONST
        //NOT PRINTABLE CHARS
        //magic_code [nbvfrt]
        switch (str[2]) {
        case 'n':
            cconst = '\n';
            break;
        case 'b':
            cconst = '\b';
            break;
        case 'v':
            cconst = '\v';
            break;
        case 'f':
            cconst = '\f';
            break;
        case 'r':
            cconst = '\r';
            break;
        case 't':
            cconst = '\t';
            break;
        }
    }
    return cconst;
}
