%{
#include <stdio.h>
#include <stdlib.h>

#include <libgen.h> //basename()

#include "build_flags.h"
#include "semantics.h"
#include "symbol_table.h"
#include "datatypes.h"
#include "scope.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "subprograms.h"
#include "subprograms_toolbox.h"
#include "ir.h"
#include "statements.h"
#include "err_buff.h"
#include "generator.h"
#include "ir_parser.h"
#include "main_app.h"
#include "final_code.h"

    int yylex(void);
    void yyerror(const char *msg);
    extern FILE *yyin;
    extern int yylineno;

    char *current_file;
    %}

%union
 {
     struct expr_t *expr;
     struct elexpr_t *elexpr;
     struct data_t *type;
     struct var_t *var;
     struct func_t *subprogram;
     struct sem_t *info;
     struct iter_t *iter_space;
     struct dim_t *dim;
     //struct ir_node_t *ir_node;
     struct statement_t *statement;

     struct expr_list_t *expr_list;
     struct var_list_t *var_list;
     struct elexpr_list_t *elexpr_list;
     struct param_list_t *param_list;

     enum pass_t pass_mode;
     enum op_t op; //operator ID

     char *str;
     float fval; //float value
     int ival; //int value
     char cval; //char value
}

%token PROGRAM
%token CONST
%token TYPE
%token ARRAY
%token SET
%token OF
%token RECORD
%token VAR
%token FORWARD
%token FUNCTION
%token PROCEDURE
%token INTEGER
%token REAL
%token BOOLEAN
%token CHAR
%token T_BEGIN
%token END
%token IF
%token THEN
%token ELSE
%token WHILE
%token DO
%token FOR
%token DOWNTO
%token TO
%token WITH
%token READ
%token WRITE

%token SEMI
%token COMMA
%token COLON
%token ASSIGN
%token DOTDOT

%token <str> ID
%token <ival> ICONST
%token <fval> RCONST
%token <ival> BCONST
%token <cval> CCONST
%token <str> STRING

 //operands
%token LBRACK RBRACK
%nonassoc <op> INOP RELOP EQU
%left <op> ADDOP OROP
%left <op> MULDIVANDOP
%nonassoc <op> NOTOP
%left DOT LBRACK RBRACK LPAREN RPAREN

 //%token COMMENT_LINE
%token ERROR_EOF_IN_STRING
%token ERROR_EOF_IN_COMMENT

%type <var> variable
%type <var> read_item

 //%type <subprogram> subprogram
 //%type <subprogram> subprograms

%type <subprogram> header
%type <iter_space> iter_space
%type <pass_mode> pass
%type <info> sub_header
%type <dim> limits
%type <elexpr> elexpression

%type <expr_list> expressions
%type <expr_list> write_list
%type <elexpr_list> elexpressions
%type <elexpr_list> setexpression

%type <var_list> read_list

%type <param_list> parameter_list
%type <param_list> formal_parameters

%type <expr> write_item
%type <expr> expression
%type <expr> constant //this is a hardcoded constant
%type <expr> constant_defs
%type <expr> field
%type <expr> limit

%type <type> typename
%type <type> standard_type
%type <type> type_def

%type <statement> statements
%type <statement> statement
%type <statement> assignment
%type <statement> if_statement
%type <statement> while_statement
%type <statement> for_statement
%type <statement> with_statement
%type <statement> subprogram_call
%type <statement> io_statement
%type <statement> comp_statement
%type <statement> if_tail

%error-verbose
%start program
%%

program: header declarations subprograms comp_statement DOT {link_statement_to_module_and_return($1,$4);}
;

header: PROGRAM ID SEMI {$$ = create_main_program($2);}
;

declarations: constdefs typedefs vardefs
;

constdefs: CONST constant_defs SEMI
| /* empty */
;

constant_defs: constant_defs SEMI ID EQU expression {declare_consts($3,$5);}
| ID EQU expression {declare_consts($1,$3);}
;

expression: expression RELOP expression {$$ = expr_relop_equ_addop_mult($1,$2,$3);}
| expression EQU expression {$$ = expr_relop_equ_addop_mult($1,$2,$3);}
| expression INOP expression {$$ = expr_inop($1,$2,$3);}
| expression OROP expression {$$ = expr_orop_andop_notop($1,$2,$3);}
| expression ADDOP expression {$$ = expr_relop_equ_addop_mult($1,$2,$3);}
| expression MULDIVANDOP expression {$$ = expr_muldivandop($1,$2,$3);}
| ADDOP expression {$$ = expr_sign($1,$2);}
| NOTOP expression {$$ = expr_orop_andop_notop(NULL,$1,$2);}
| variable {$$ = expr_from_variable($1);}
| ID LPAREN expressions RPAREN {$$ = expr_from_function_call($1,$3);}
| constant {$$ = $1;}//hardcoded constants are expressions
| LPAREN expression RPAREN { $$ = $2; }
| setexpression {$$ = expr_from_setexpression($1);}
;

variable: ID {$$ = reference_to_variable_or_enum_element($1);}
| variable DOT ID {$$ = reference_to_record_element($1,$3);}
| variable LBRACK expressions RBRACK {$$ = reference_to_array_element($1,$3);}
;

expressions: expressions COMMA expression {$$ = expr_list_add($1,$3);}//pass array's dimensions or function's parameters
| expression {$$ = expr_list_add(NULL,$1);}
;

constant: ICONST {$$ = expr_from_hardcoded_int($1);}
| RCONST {$$ = expr_from_hardcoded_real($1);}
| BCONST {$$ = expr_from_hardcoded_boolean($1);}
| CCONST {$$ = expr_from_hardcoded_char($1);}
;

setexpression: LBRACK elexpressions RBRACK {$$ = $2;}
| LBRACK RBRACK {$$ = NULL;}
;

elexpressions: elexpressions COMMA elexpression {$$ = elexpr_list_add($1,$3);}//assign values to a set type
| elexpression {$$ = elexpr_list_add(NULL,$1);}
;

elexpression: expression DOTDOT expression {$$ = make_elexpr_range($1,$3);}//assign values to a set type
| expression {$$ = make_elexpr($1);}//assign values to a set type
;

typedefs: TYPE type_defs SEMI
| /* empty */
;

type_defs: type_defs SEMI ID EQU type_def {make_type_definition($3,$5);}
| ID EQU type_def {make_type_definition($1,$3);}
;

type_def: ARRAY LBRACK dims RBRACK OF typename {$$ = close_array_type($6);}
| SET OF typename {$$ = close_set_type($3);}
| RECORD fields END {$$ = close_record_type();}
| LPAREN identifiers RPAREN {$$ = close_enum_type();}//this is enum type
| limit DOTDOT limit {$$ = close_subset_type($1,$3);}//subset definition
;

dims: dims COMMA limits {add_dim_to_array_type($3);}
| limits {add_dim_to_array_type($1);}
;

limits: limit DOTDOT limit {$$ = make_dim_bounds($1,$3);}
| ID {$$ = make_dim_bound_from_id($1);}
;

limit: ADDOP ICONST {$$ = expr_from_signed_hardcoded_int(ADDOP,$2);}
| ADDOP ID {$$ = limit_from_signed_id($1,$2);}
| ICONST {$$ = expr_from_hardcoded_int($1);}
| CCONST {$$ = expr_from_hardcoded_char($1);}
| BCONST {$$ = expr_from_hardcoded_boolean($1);}
| ID {$$ = limit_from_id($1);}
;

typename: standard_type {$$ = $1;}
| ID {$$ = reference_to_typename($1);}
;

standard_type: INTEGER {$$ = SEM_INTEGER;}
| REAL {$$ = SEM_REAL;}
| BOOLEAN  {$$ = SEM_BOOLEAN;}
| CHAR {$$ = SEM_CHAR;}
;

fields: fields SEMI field {idf_addto_record();}//record (struct) elements
| field {idf_addto_record();}
;

field: identifiers COLON typename {idf_set_type($3);}
;

identifiers: identifiers COMMA ID {idf_insert($3);}//used for enumerations, formal_parameters and variable declarations
| ID {idf_insert($1);}
;

vardefs: VAR variable_defs SEMI
| /* empty */
;

variable_defs: variable_defs SEMI identifiers COLON typename {declare_vars($5);}
| identifiers COLON typename {declare_vars($3);}
;

subprograms: subprograms subprogram SEMI
| /* empty */
;

subprogram: sub_header SEMI FORWARD {forward_subprogram_declaration($1);}
| sub_header SEMI {subprogram_init($1);} declarations subprograms comp_statement {subprogram_finit($1,$6);}
;

sub_header: FUNCTION ID formal_parameters COLON standard_type {$$ = declare_function_header($2,$3,$5);}
| PROCEDURE ID formal_parameters {$$ = declare_procedure_header($2,$3);}
| FUNCTION ID {$$ = reference_to_forwarded_function($2);}
;

formal_parameters: LPAREN parameter_list RPAREN {$$ = $2;}//formal_parameters go to stack
| /* empty */ {$$ = NULL;}
;

parameter_list: parameter_list SEMI pass identifiers COLON typename {$$ = param_insert($1,$3,$6);}
| pass identifiers COLON typename {$$ = param_insert(NULL,$1,$4);}
;

pass: VAR {$$ = PASS_REF;}
| /* empty */ {$$ = PASS_VAL;}
;

comp_statement: T_BEGIN statements END {$$ = $2;}
;

statements: statements SEMI statement {$$ = link_statements($3,$1);}
| statement {$$ = $1;}
;

statement: assignment
| if_statement
| while_statement
| for_statement
| with_statement
| subprogram_call
| io_statement
| comp_statement
| /* empty */ {$$ = NULL;}
;

assignment: variable ASSIGN expression {$$ = statement_assignment($1,$3);}
| variable ASSIGN STRING {$$ = statement_assignment($1,expr_from_STRING($3));}
;

if_statement: IF expression THEN {prepare_branch_stmt($2);} statement if_tail {$$ = statement_if($2,$5,$6);}
;

if_tail: ELSE statement {$$ = $2;}
| /* empty */ {$$ = NULL;}
;

while_statement: WHILE expression {prepare_branch_stmt($2);} DO statement {$$ = statement_while($2,$5);}
;

for_statement: FOR ID ASSIGN iter_space DO {$<var>$ = protect_guard_var($2);} statement {$$ = statement_for($<var>6,$4,$7);}
;

iter_space: expression TO expression {$$ = make_iter_space($1,1,$3);}
| expression DOWNTO expression {$$ = make_iter_space($1,-1,$3);}
;

with_statement: WITH variable DO {start_new_with_statement_scope($2);} statement {$$ = statement_with($2,$5); close_last_opened_with_statement_scope();}
;

subprogram_call: ID {func_t *s = find_subprogram($1);$$ = statement_call(s,NULL);}
| ID LPAREN expressions RPAREN {func_t *s = find_subprogram($1);$$ = statement_call(s,$3);}
;

io_statement: READ LPAREN read_list RPAREN {$$ = statement_read($3);}
| WRITE LPAREN write_list RPAREN {$$ = statement_write($3);}
;

read_list: read_list COMMA read_item {$$ = var_list_add($1,$3);}
| read_item {$$ = var_list_add(NULL,$1);}
;

read_item: variable
;

write_list: write_list COMMA write_item {$$ = expr_list_add($1,$3);}
| write_item {$$ = expr_list_add(NULL,$1);}
;

write_item: expression
| STRING {$$ = expr_from_STRING($1);}
;

%%

#if LEX_MAIN == 0

int main(int argc, char *argv[]) {
    int status;

    init_err_buff();
    init_symbol_table();
    init_statements();
    init_ir();
    init_final_code();

    if (argc>1) {
        if (argc!=2) {
            printf("Ignoring multiple arguments\n"); //FIXME parse all the arguments
        }
        yyin = fopen(argv[argc-1],"r");
        if (!yyin) {
            perror(NULL);
            exit(EXIT_FAILURE);
        }
        current_file = basename(argv[argc-1]);
        status = yyparse();
        fclose(yyin);
    }
    else {
        help();
        exit(EXIT_FAILURE);
    }

    switch (status) {
    case 0:
        if (!err_num) {
            /* Frontend @ statement_t *statement_root_module[MAX_NUM_OF_MODULES]; */



            /* high level optimizations go here */



            /* EOF (end of frontend) :) */
            generate_all_modules();
            /* IR @ ir_node_t *ir_root_tree[MAX_NUM_OF_MODULES]; */



            /* IR optimizations go here */



            /* End of IR */
            parse_all_modules();
            /* Backend @ instr_t *final_tree[MAX_NUM_OF_MODULES]; */



            /* low level optimizations go here */



            /* End of Backend, Emit final code */
            print_assembly();

            exit(EXIT_SUCCESS);
        } else {
            printf("Please correct the errors.\n");
            exit(EXIT_FAILURE);
        }
    case 1:
    case 2:
    default: exit(EXIT_FAILURE);
    }
}

void yyerror(const char *msg) {
    err_num++;

    printf("%s:%d:error: %s\n",current_file,yylineno,msg);

#if LOG_TO_FILE == 1
    fprintf(log_file,"%s\n",msg);
#endif

    if (ERR_NUM_LIMIT>0 && err_num >= ERR_NUM_LIMIT) {
        printf("ERROR: So many errors, abord parsing.\n");
#if LOG_TO_FILE == 1
        fprintf(log_file,"ERROR: So many errors, abord parsing.\n");
#endif
        exit(EXIT_FAILURE);
    }
}

#endif
