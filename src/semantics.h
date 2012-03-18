/* Here are some additional declarations of types needed for the declaration of
 * the YYSTYPE union. For this reason this header must be #included before the
 * declaration of the YYSTYPE union, which is in the bison.tab.h header.
 */

#ifndef _SEMANTICS_H
#define _SEMANTICS_H

//set data types have a limit of 128 different objects :fixme
#define MAX_FIELDS 128
#define MAX_PARAMS 128
#define MAX_SET_ELEM 128
#define MAX_ARRAY_DIMS 8
#define MAX_EXPR_LIST (MAX_PARAMS>MAX_ARRAY_DIMS?MAX_PARAMS:MAX_ARRAY_DIMS)
#define MAX_VAR_LIST MAX_EXPR_LIST
#define MAX_SUBPROGRAMS_IN_SCOPE MAX_EXPR_LIST

//some structs contain elements of a struct that is declared later, so this keeps the compiler happy
struct expr_t;
struct var_t;
struct func_t;
struct elexpr_list_t;
struct expr_list_t;
struct sem_t;

/** info `enum idt_t`
 * when a sub_header is declared it is marked as
 * ID_FORWARDED_FUNC or ID_FORWARDED_PROC for functions
 * and procedures respectively. They become ID_FUNC or
 * ID_PROC when the body of subprogram is defined. If a
 * subprogram is not `_FORWARDED_` it means that it
 * already has a body, so we define a body for a
 * subprogram only if the sub_header is marked as
 * `_FORWARDED_`. Thats because multiple body
 * definitions for a sub_header are forbidden.
 */
typedef enum idt_t {
    ID_VAR,
    ID_VAR_GUARDED, //this variable controls a `for` loop statement
    ID_LOST,	//for undeclared symbols, to avoid unreal error messages
    ID_CONST,
    ID_STRING,	//only for constant strings
    ID_RETURN,	//return value of func
    ID_FUNC,
    ID_PROC,
    ID_FORWARDED_FUNC,
    ID_FORWARDED_PROC,
    ID_TYPEDEF,
    ID_PROGRAM_NAME
} idt_t;

typedef enum type_t {
    TYPE_VOID,
    TYPE_INT,
    TYPE_REAL,
    TYPE_BOOLEAN,
    TYPE_CHAR,
    TYPE_ENUM,
    TYPE_SUBSET,
    TYPE_ARRAY,
    TYPE_RECORD,
    TYPE_SET,
} type_t;

typedef enum var_status_value_t {
    VALUE_GARBAGE,
    VALUE_VALID
} var_status_value_t;

typedef enum var_status_use_t {
    USE_NONE,
    USE_YES
} var_status_use_t;

typedef enum var_status_known_t {
    KNOWN_NO,
    KNOWN_YES
} var_status_known_t;

typedef enum func_status_t {
    FUNC_USEFULL,
    FUNC_OBSOLETE
} func_status_t;

typedef enum mem_seg_t {
    MEM_GLOBAL,
    MEM_STACK,
    MEM_REGISTER,	//for formal parameters only
} mem_seg_t;

/** Dimension of array types
 * `datatype` element's type is data_t, but this struct is declared later
 */
typedef struct dim_t {
    int first;
    int range;
    int relative_distance; //number of elements between 2 indexes of the same dimension
} dim_t;

typedef struct symbol_table_t {
    int start_index; //for array symbol tables

    char **lost;
    int lost_empty;

    struct sem_t **pool;
    int pool_empty;

} symbol_table_t;

typedef struct data_t {
    type_t is;
    char *name; //name of typedef
    struct data_t *def_datatype; //type of array's or set elemets, (enumerations set this to integer type)
    int field_num; //number of: elements in record type, array dimensions, enum or subset elements
    char *field_name[MAX_FIELDS]; //enum names, record elements
    int field_offset[MAX_FIELDS]; //only for records: element's relative position from the start of the record
    struct data_t *field_datatype[MAX_FIELDS]; //only for records: record element's datatype
    int enum_num[MAX_FIELDS]; //only for enums and subsets: each number of field_name
    dim_t *dim[MAX_ARRAY_DIMS]; //only for arrays: array's dimensions
    int memsize; //sizeof data type in bytes
    //scope_t *scope; //depth of declaration
    struct func_t *scope; //depth of declaration
} data_t;

#define TYPE_IS_STANDARD(d) (d->is==TYPE_INT || d->is==TYPE_REAL || d->is==TYPE_BOOLEAN || d->is==TYPE_CHAR)
#define TYPE_IS_SCALAR(d) (d->is==TYPE_INT || d->is==TYPE_CHAR || d->is==TYPE_BOOLEAN || d->is==TYPE_ENUM || d->is==TYPE_SUBSET)
#define TYPE_IS_ARITHMETIC(d) (d->is==TYPE_INT || d->is==TYPE_REAL)
#define TYPE_IS_COMPOSITE(d) (d->is==TYPE_ARRAY || d->is==TYPE_RECORD || d->is==TYPE_SET)
#define TYPE_IS_ELEXPR_VALID(d) (d->is==TYPE_CHAR || d->is==TYPE_BOOLEAN || d->is==TYPE_ENUM || d->is==TYPE_SUBSET)
#define TYPE_IS_SUBSET_VALID(d) (d->is==TYPE_INT || d->is==TYPE_CHAR || d->is==TYPE_ENUM)
#define TYPE_IS_STRING(d) (d==VIRTUAL_STRING_DATATYPE || (d->is==TYPE_ARRAY && d->field_num==1 && d->def_datatype->is==TYPE_CHAR))

typedef enum pass_t {
    PASS_VAL,
    PASS_REF,
} pass_t;

/** Mem_t struct
 * `offset_expr` element's type is expr_t but this  struct is defined later
 */
typedef struct mem_t {
    mem_seg_t segment; //global or local (stack) scope relevant
    int direct_register_number; //the max is MAX_FORMAL_PARAMETERS_FOR_DIRECT_PASS, see mem.h
    struct expr_t * seg_offset; //static hardcoded relative distance (integer) from segment's start (doesn't change)
    struct expr_t *offset_expr; //dynamic code to calculate relative distance from variable's start (for arrays or records)
    pass_t content_type; //if PASS_VAL the memory contains the value, if PASS_REF the memory contains the address (for pointers)
    int size; //memory size
} mem_t;

typedef struct info_comp_t {
    struct var_t *base;              //array/record base variable
    struct expr_list_t *index_list;  //array index
    int element;                     //record element offset (hardcoded)
} info_comp_t;

/** Variables & declared Constants struct
 * We use the var_t struct to represent both variables and declared constants in the symbol table.
 */
typedef struct var_t {
    idt_t id_is; //ID_VAR, ID_VAR_GUARDED, ID_CONST, ID_RETURN, ID_LOST

    var_status_value_t status_value;
    var_status_use_t status_use;
    var_status_known_t status_known;

    info_comp_t *from_comp;

    data_t *datatype;
    char *name;
    //scope_t *scope;
    struct func_t *scope;
    mem_t *Lvalue; //for formal parameters in symbol table, if Lvalue is NULL the variable is passed by value, else by reference and we load it from here
    struct expr_t *cond_assign; //if not NULL, check this cond to assign
    float fval; //hardcoded float value
    int ival; //hardcoded int value
    char cval; //hardcoded char value
    char *cstr; //hardcoded string
} var_t;

typedef struct var_list_t {
    var_t *var_list[MAX_VAR_LIST];
    int var_list_empty;
    int all_var_num; //the supposed number of variables
} var_list_t;

typedef struct param_t {
    char *name; //name of each parameter
    pass_t pass_mode;
    data_t *datatype; //type of each parameter
} param_t;

typedef struct param_list_t {
    param_t *param[MAX_PARAMS];
    int param_empty;
} param_list_t;

typedef struct func_t {
    func_status_t status;
    var_t *return_value;
    char *name;
    int param_num; //number of parameters
    param_t *param[MAX_PARAMS];
    mem_t *param_Lvalue[MAX_PARAMS];
    int stack_size; //formal parameters + return value + whatever
    struct func_t *scope;
    symbol_table_t symbol_table;
    char *label;
    int unique_id; //index of ir_root_tree (low level info)
} func_t;

typedef enum op_t {
    OP_IGNORE,
    RELOP_B,	// '>'
    RELOP_BE,	// '>='
    RELOP_L,	// '<'
    RELOP_LE,	// '<='
    RELOP_NE,	// '<>'
    RELOP_EQU,	// '='
    RELOP_IN,	// 'in'
    OP_SIGN, 	//dummy operator (OP_MINUS synonym), to determine when OP_MINUS is used as sign, (we ignore OP_PLUS)
    OP_PLUS,	// '+'
    OP_MINUS,	// '-'
    OP_MULT,	// '*'
    OP_RDIV,	// '/'
    OP_DIV,	// 'div'
    OP_MOD,	// 'mod'
    OP_AND,	// 'and'
    OP_OR,     	// 'or'
    OP_NOT     	// 'not'
} op_t;

typedef enum expr_type_t {
    EXPR_RVAL,
    EXPR_LVAL,
    EXPR_HARDCODED_CONST, //only for standard types
    EXPR_STRING,    //only for hardcoded STRINGS
    EXPR_SET,
    EXPR_NULL_SET,
    EXPR_LOST,
} expr_type_t;

/** Expressions struct
 * we work with `datatype` element of struct expr_t when we
 * need to know the type of expression. if the expr is a
 * variable or a declared constant, the var->datatype is the same
 * with datatype. the `var_t` elements exists in case we want more information about the variable.
 */
typedef struct expr_t {
    expr_type_t expr_is;
    op_t op; //the expression type
    var_t *var;
    data_t *datatype; //type of exression or of hardcoded constant or of return value
    data_t *convert_to; //int->real, real->int
    struct elexpr_list_t *elexpr_list;
    struct expr_list_t *expr_list; //for function calls
    float fval; //float value
    int ival; //int value
    char cval; //char value
    char *cstr; //string constant
    struct expr_t *parent;
    struct expr_t *l1;
    struct expr_t *l2;
} expr_t;

typedef struct expr_list_t {
    expr_t *expr_list[MAX_EXPR_LIST];
    int expr_list_empty;
    int all_expr_num; //the supposed number of expressions
} expr_list_t;

/** Loop range in a `for` statement */
typedef struct iter_t {
    struct expr_t *start;
    struct expr_t *stop;
    struct expr_t *step;
} iter_t;

/** Set element struct */
typedef struct elexpr_t {
    expr_t *left;
    expr_t *right;
    data_t *elexpr_datatype;
} elexpr_t;

typedef struct elexpr_list_t {
    expr_type_t elexpr_list_usage;
    elexpr_t *elexpr_list[MAX_SET_ELEM];
    int elexpr_list_empty;
    int all_elexpr_num;
    data_t *elexpr_list_datatype;
} elexpr_list_t;

#endif
