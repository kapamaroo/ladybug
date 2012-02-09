%{
#include "build_flags.h"
#include "semantics.h" //Include it before the declaration of YYSTYPE
#include "bison.tab.h"
#include "FuncTools.h"

#if LOG_TO_FILE == 1
#include "err_buff.h"
#endif

void ERROR(const char *msg);

//set yylval here and a main() at the bootom if there is only the lexical analyzer
#if LEX_MAIN == 1
  YYSTYPE yylval;
#endif

%}

%option case-insensitive
%option yylineno
%option noyywrap
letter [A-Z]
digit [0-9]
digit_nz [1-9]
hex [A-F0-9]
hex_nz [A-F1-9]
bin [01]
bin_nz [1]
alphanum [A-Z0-9]
magic_char [\b\v\f\r\t]
magic_code [nbvfrt]
line_feed [\n]

%x string
%x comment

%%

"program" {
#if LEX_DEBUG == 1
  printf("PROGRAM : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"PROGRAM : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (PROGRAM);
}

"const" {
#if LEX_DEBUG == 1
  printf("CONST : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"CONST : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (CONST);
}

"type" {
#if LEX_DEBUG == 1
  printf("TYPE : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"TYPE : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (TYPE);
}

"array" {
#if LEX_DEBUG == 1
  printf("ARRAY : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"ARRAY : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (ARRAY);
}

"set" {
#if LEX_DEBUG == 1
  printf("SET : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"SET : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (SET);
}

"of" {
#if LEX_DEBUG == 1
  printf("OF : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"OF : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (OF);
}

"record" {
#if LEX_DEBUG == 1
  printf("RECORD : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RECORD : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (RECORD);
}

"var" {
#if LEX_DEBUG == 1
  printf("VAR : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"VAR : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (VAR);
}

"forward" {
#if LEX_DEBUG == 1
  printf("FORWARD : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"FORWARD : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (FORWARD);
}

"function" {
#if LEX_DEBUG == 1
  printf("FUNCTION : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"FUNCTION : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (FUNCTION);
}

"procedure" {
#if LEX_DEBUG == 1
  printf("PROCEDURE : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"PROCEDURE : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (PROCEDURE);
}

"integer" {
#if LEX_DEBUG == 1
  printf("INTEGER : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"INTEGER : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (INTEGER);
}

"real" {
#if LEX_DEBUG == 1
  printf("REAL : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"REAL : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (REAL);
}

"boolean" {
#if LEX_DEBUG == 1
  printf("BOOLEAN : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"BOOLEAN : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (BOOLEAN);
}

"char" {
#if LEX_DEBUG == 1
  printf("CHAR : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"CHAR : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (CHAR);
}

"begin" {
#if LEX_DEBUG == 1
  printf("T_BEGIN : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"T_BEGIN : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (T_BEGIN);
}

"end" {
#if LEX_DEBUG == 1
  printf("END : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"END : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (END);
}

"if" {
#if LEX_DEBUG == 1
  printf("IF : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"IF : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (IF);
}

"then" {
#if LEX_DEBUG == 1
  printf("THEN : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"THEN : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (THEN);
}

"else" {
#if LEX_DEBUG == 1
  printf("ELSE : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"ELSE : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (ELSE);
}

"while" {
#if LEX_DEBUG == 1
  printf("WHILE : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"WHILE : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (WHILE);
}

"do" {
#if LEX_DEBUG == 1
  printf("DO : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"DO : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (DO);
}

"for" {
#if LEX_DEBUG == 1
  printf("FOR : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"FOR : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (FOR);
}

"downto" {
#if LEX_DEBUG == 1
  printf("DOWNTO : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"DOWNTO : %s\n",yytext);
#endif
#endif
  //yylval.op = ITER_DOWNTO;
  bufadd(&line_buf,yytext);
  return (DOWNTO);
}

"to" {
#if LEX_DEBUG == 1
  printf("TO : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"TO : %s\n",yytext);
#endif
#endif
  //yylval.op = ITER_TO;
  bufadd(&line_buf,yytext);
  return (TO);
}

"with" {
#if LEX_DEBUG == 1
  printf("WITH : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"WITH : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (WITH);
}

"read" {
#if LEX_DEBUG == 1
  printf("READ : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"READ : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (READ);
}

"write" {
#if LEX_DEBUG == 1
  printf("WRITE : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"WRITE : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (WRITE);
}


"true" {
#if LEX_DEBUG == 1
  printf("BCONST : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"BCONST : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  yylval.ival = 1;
  bufadd(&line_buf,yytext);
  return (BCONST);
}

"false" {
#if LEX_DEBUG == 1
  printf("BCONST : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"BCONST : %s\n",yytext);
#endif
#endif
  //yylval.str = yytext;
  yylval.ival = 0;
  bufadd(&line_buf,yytext);
  return (BCONST);
}

">"  {
#if LEX_DEBUG == 1
  printf("RELOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RELOP : %s\n",yytext);
#endif
#endif
  yylval.op = RELOP_B;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (RELOP);
}

">=" {
#if LEX_DEBUG == 1
  printf("RELOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RELOP : %s\n",yytext);
#endif
#endif
  yylval.op = RELOP_BE;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (RELOP);
}

"<"  {
#if LEX_DEBUG == 1
  printf("RELOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RELOP : %s\n",yytext);
#endif
#endif
  yylval.op = RELOP_L;
    //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (RELOP);
}

"<=" {
#if LEX_DEBUG == 1
  printf("RELOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RELOP : %s\n",yytext);
#endif
#endif
  yylval.op = RELOP_LE;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (RELOP);
}

"<>" {
#if LEX_DEBUG == 1
  printf("RELOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RELOP : %s\n",yytext);
#endif
#endif
  yylval.op = RELOP_NE;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (RELOP);
}

"+" {
#if LEX_DEBUG == 1
  printf("ADDOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"ADDOP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_PLUS;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (ADDOP);
}

"-" {
#if LEX_DEBUG == 1
  printf("ADDOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"ADDOP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_MINUS;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (ADDOP);
}

"or" {
#if LEX_DEBUG == 1
  printf("OROP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"OROP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_OR;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (OROP);
}

"*"   {
#if LEX_DEBUG == 1
  printf("MULDIVANDOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"MULDIVANDOP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_MULT;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (MULDIVANDOP);
}

"/"   {
#if LEX_DEBUG == 1
  printf("MULDIVANDOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"MULDIVANDOP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_RDIV;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (MULDIVANDOP);
}

"div" {
#if LEX_DEBUG == 1
  printf("MULDIVANDOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"MULDIVANDOP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_DIV;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (MULDIVANDOP);
}

"mod" {
#if LEX_DEBUG == 1
  printf("MULDIVANDOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"MULDIVANDOP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_MOD;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (MULDIVANDOP);
}

"and" {
#if LEX_DEBUG == 1
  printf("MULDIVANDOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"MULDIVANDOP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_AND;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (MULDIVANDOP);
}

"not" {
#if LEX_DEBUG == 1
  printf("NOTOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"NOTOP : %s\n",yytext);
#endif
#endif
  yylval.op = OP_NOT;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (NOTOP);
}

"in" {
#if LEX_DEBUG == 1
  printf("INOP : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"INOP : %s\n",yytext);
#endif
#endif
  yylval.op = RELOP_IN;
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (INOP);
}

[_]?{letter}(({alphanum}|[_])*{alphanum}+)? {
#if LEX_DEBUG == 1
  printf("ID : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"ID : %s\n",yytext);
#endif
#endif
  yylval.str = strdup(yytext);
  bufadd(&line_buf,yytext);
  return (ID);
}

[0]|({digit_nz}{digit}*) {
  yylval.ival = DecToDec(yytext);
  //yylval.type = TYPE_INT;
#if LEX_DEBUG == 1
  printf("ICONST : %s yylval %d\n",yytext,yylval.ival);
#if LOG_TO_FILE == 1
  fprintf(log_file,"ICONST : %s yylval %d\n",yytext,yylval.ival);
#endif
#endif
  bufadd(&line_buf,yytext);
  return (ICONST);
}

[0][H]({hex_nz}{hex}*) {
  yylval.ival = HexToDec((yytext+2));
  //yylval.type = TYPE_INT;
#if LEX_DEBUG == 1
  printf("ICONST : %s yylval %d\n",yytext,yylval.ival);
#if LOG_TO_FILE == 1
  fprintf(log_file,"ICONST : %s yylval %d\n",yytext,yylval.ival);
#endif
#endif
  bufadd(&line_buf,yytext);
  return (ICONST);
}

[0][B]({bin_nz}{bin}*) {
  yylval.ival = BinToDec((yytext+2));
  //yylval.type = TYPE_INT;
#if LEX_DEBUG == 1
  printf("ICONST : %s yylval %d\n",yytext,yylval.ival);
#if LOG_TO_FILE == 1
  fprintf(log_file,"ICONST : %s yylval %d\n",yytext,yylval.ival);
#endif
#endif
  bufadd(&line_buf,yytext);
  return (ICONST);
}

[0][H]({hex_nz}{hex}*)*"."([0]|[0]*{hex_nz}+{hex}*) {
  yylval.fval = strHexFloatToDecFloat((yytext+2));
  //yylval.type = TYPE_REAL;
#if LEX_DEBUG == 1
  printf("RCONST : %s yylval %f\n",yytext,yylval.fval);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RCONST : %s yylval %f\n",yytext,yylval.fval);
#endif
#endif
  bufadd(&line_buf,yytext);
  return (RCONST);
}

[0][B]({bin_nz}{bin}*)*"."([0]|[0]*{bin_nz}+{bin}*) {
  yylval.fval = strBinFloatToDecFloat((yytext+2));
  //yylval.type = TYPE_REAL;
#if LEX_DEBUG == 1
  printf("RCONST : %s yylval %f\n",yytext,yylval.fval);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RCONST : %s yylval %f\n",yytext,yylval.fval);
#endif
#endif
  bufadd(&line_buf,yytext);
  return (RCONST);
}

((([0]|{digit_nz}{digit}*)("."([0]|[0]*{digit_nz}+{digit}*))?)|(([0]|{digit_nz}{digit}*)?("."([0]|[0]*{digit_nz}+{digit}*))))([E][+-]?([0]|({digit_nz}{digit}*)))? {
  yylval.fval = strFloatToDecFloat(yytext);
  //yylval.type = TYPE_REAL;
#if LEX_DEBUG == 1
  printf("RCONST : %s yylval %f\n",yytext,yylval.fval);
#if LOG_TO_FILE == 1
  fprintf(log_file,"RCONST : %s yylval %f\n",yytext,yylval.fval);
#endif
#endif
  bufadd(&line_buf,yytext);
  return (RCONST);
}

[']([\x20-\x7E]|([\\]({magic_code})))['] {
#if LEX_DEBUG == 1
  printf("CCONST : %s\n",yytext);
#if LOG_TO_FILE == 1
  fprintf(log_file,"CCONST : %s\n",yytext);
#endif
#endif
  //yylval.type = TYPE_CHAR;
  //yylval.str = strdup(yytext); //FIXME
  yylval.cval = isolate_char_const(yytext);
  bufadd(&line_buf,yytext);
  return (CCONST);
}

[(] {
#if LEX_DEBUG == 1
  printf("LPAREN\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"LPAREN\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (LPAREN);
}

[)] {
#if LEX_DEBUG == 1
  printf("RPAREN\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"RPAREN\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (RPAREN);
}

[;] {
#if LEX_DEBUG == 1
  printf("SEMI\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"SEMI\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (SEMI);
}

[.] {
#if LEX_DEBUG == 1
  printf("DOT\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"DOT\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (DOT);
}

[,] {
#if LEX_DEBUG == 1
  printf("COMMA\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"COMMA\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (COMMA);
}

[=] {
#if LEX_DEBUG == 1
  printf("EQU\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"EQU\n");
#endif
#endif
  yylval.op = RELOP_EQU;
  bufadd(&line_buf,yytext);
  return (EQU);
}

[:] {
#if LEX_DEBUG == 1
  printf("COLON\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"COLON\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (COLON);
}

"[" {
#if LEX_DEBUG == 1
  printf("LBRACK\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"LBRACK\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (LBRACK);
}

"]" {
#if LEX_DEBUG == 1
  printf("RBRACK\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"RBRACK\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (RBRACK);
}

":=" {
#if LEX_DEBUG == 1
  printf("ASSIGN\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"ASSIGN\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (ASSIGN);
}

".." {
#if LEX_DEBUG == 1
  printf("DOTDOT\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"DOTDOT\n");
#endif
#endif
  //yylval.str = yytext;
  bufadd(&line_buf,yytext);
  return (DOTDOT);
}

"\"" {
  init_buf(&str_buf);
  bufadd(&line_buf,yytext);
  BEGIN(string);
}

"\{" {
  bufadd(&line_buf,yytext);
  BEGIN(comment);
#if LEX_DEBUG == 1
  printf("START_COMMENT\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"START_COMMENT\n");
#endif
#endif
}

[ \t]+ {
  bufadd(&line_buf,yytext);
} //ignore white characters and separate tokens

{line_feed}+ {
  init_buf(&line_buf);
}

. {
  bufadd(&line_buf,yytext);
  ERROR("Invalid character");
}

<INITIAL><<EOF>> {
  yyterminate();
}

<string>{
  "\"" {
    *str_buf.ptr = '\0';
    yylval.str = str_buf.buf;
#if LEX_DEBUG == 1
    printf("STRING : \"%s\"\n",yylval.str);
#if LOG_TO_FILE == 1
  fprintf(log_file,"STRING : \"%s\"\n",yylval.str);
#endif
#endif
    bufadd(&line_buf,yytext);
    BEGIN(INITIAL);
    return (STRING);
  }

  {line_feed} {
    ERROR("Unescaped new line inside string.");
    init_buf(&line_buf);
  }

  ([\\]{line_feed})+ {
    init_buf(&line_buf);
  }

  ([\\][\"\\])+ {
    bufadd(&str_buf,yytext);
    bufadd(&line_buf,yytext);
  }

  [\\]?[^\"\n\\]+ {
    bufadd(&str_buf,yytext);
    bufadd(&line_buf,yytext);
  }

  <<EOF>> {
    ERROR("Unexpected EOF inside string.");
    return (ERROR_EOF_IN_STRING);
  }
}

<comment>{
  [\}] {
#if LEX_DEBUG == 1
    printf("END_OF_COMMENT\n");
#if LOG_TO_FILE == 1
  fprintf(log_file,"END_OF_COMMENT\n");
#endif
#endif
    BEGIN(INITIAL);
  }

  [^\}\n]+ {;}
  [\n]+ {;} //separate line feed for preformance reasons

  <<EOF>> {
    ERROR("Unexpected EOF in comment.");
    return (ERROR_EOF_IN_COMMENT);
  }
}

%%
void ERROR(const char* msg) {
  printf("ERROR: %s\nLine %d: %s\n",msg,yylineno,line_buf.buf);
}
