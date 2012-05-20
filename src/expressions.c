#include <stdio.h>
#include <stdlib.h>

#include "semantics.h"
#include "symbol_table.h"
#include "datatypes.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "err_buff.h"

int expr_lval_with_comp_datatype(expr_t *l);
expr_t *expr_simplify_hardcoded(expr_t *new_expr, expr_t *l1, op_t op, expr_t *l2);

/** Expression routines */
expr_t *expr_relop_equ_addop_mult(expr_t *l1,op_t op,expr_t *l2) {
    //relops need paren
    expr_t *new_expr;
    data_t *datatype;

    if (!l1 || !l2) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("null expression in expr_relop_equ (debugging info)");
#endif
        return expr_from_hardcoded_boolean(0);
    }

    if (l1->expr_is==EXPR_LOST || l2->expr_is==EXPR_LOST) {
        switch (op) {
        case OP_PLUS:
        case OP_MINUS:
        case OP_MULT:
            return expr_from_hardcoded_int(0);
        default:
            return expr_from_hardcoded_boolean(0);
        }
    }

    //reminder: we cannot have STRING inside expressions, see bison.y

    if ((l1->expr_is==EXPR_SET || l1->expr_is==EXPR_NULL_SET ||
         (l1->expr_is==EXPR_LVAL && l1->datatype->is==TYPE_SET)) &&
        (l2->expr_is==EXPR_SET || l2->expr_is==EXPR_NULL_SET ||
         (l2->expr_is==EXPR_LVAL && l2->datatype->is==TYPE_SET))) {
        //both expressions are sets, check for valid operator
        switch (op) {
        case OP_PLUS:
        case OP_MINUS:
        case OP_MULT:
            //set the datatype of the new expression
            if (l1->expr_is==EXPR_LVAL && l2->expr_is==EXPR_LVAL) {
                if (l1->datatype!=l2->datatype) {
                    yyerror("comparing sets with different data types");
                    return expr_from_setexpression(NULL); //return the NULL set expression
                }
                datatype = l1->datatype->def_datatype; //the result set has the same datadef type
            }
            else if(l1->expr_is==EXPR_LVAL) {
                if (l1->datatype->def_datatype!=l2->datatype) {
                    yyerror("comparing sets with different data types");
                    return expr_from_setexpression(NULL); //return the NULL set expression
                }
                datatype = l2->datatype; //the result set has the same datadef type
            }
            else if(l2->expr_is==EXPR_LVAL) {
                if (l1->datatype!=l2->datatype->def_datatype) {
                    yyerror("comparing sets with different data types");
                    return expr_from_setexpression(NULL); //return the NULL set expression
                }
                datatype = l1->datatype; //the result set has the same datadef type
            }
            else {
                if (l1->datatype!=l2->datatype) {
                    yyerror("comparing sets with different datadef types");
                    return expr_from_setexpression(NULL); //return the NULL set expression
                }
                datatype = l1->datatype; //the result set has the same datadef type
            }

            new_expr = (expr_t*)calloc(1,sizeof(expr_t));
            new_expr->parent = new_expr;
            new_expr->datatype = datatype;
            new_expr->expr_is = EXPR_SET;
            new_expr->op = op;
            new_expr->l1 = l1;
            new_expr->l2 = l2;
            l1->parent = new_expr;
            l2->parent = new_expr;

            return new_expr;
        default:
            yyerror("invalid operator for set types");
            return expr_from_hardcoded_boolean(0);
        }
    }

    //maybe only the one of them is a set expression
    if ((l1->expr_is==EXPR_SET || l1->expr_is==EXPR_NULL_SET ||
         (l1->expr_is==EXPR_LVAL && l1->datatype->is==TYPE_SET)) ||
        (l2->expr_is==EXPR_SET || l2->expr_is==EXPR_NULL_SET ||
         (l2->expr_is==EXPR_LVAL && l2->datatype->is==TYPE_SET))) {
        switch (op) {
        case OP_PLUS:
        case OP_MINUS:
        case OP_MULT:
            //assume that the programmer wanted an expression of sets
            yyerror("expected both expressions to be of set datatype");
            return expr_from_setexpression(NULL); //return the NULL set expression
        default:
            //all other operators in this function are comparison operators
            //assume that the programmer wanted a boolean expression
            sprintf(str_err,"'%s' operator does not apply to sets",op_literal(op));
            yyerror(str_err);
            return expr_from_hardcoded_boolean(0);
        }
    }

    //continue only with scalar or arithmetic datatypes
    if (expr_lval_with_comp_datatype(l1) || expr_lval_with_comp_datatype(l2)) {
        switch (op) {
        case OP_PLUS:
        case OP_MINUS:
        case OP_MULT:   return expr_from_hardcoded_int(0);
        default:        return expr_from_hardcoded_boolean(0);
        }
    }

    if ((l1->expr_is==EXPR_RVAL || l1->expr_is==EXPR_LVAL || l1->expr_is==EXPR_HARDCODED_CONST) &&
        (l2->expr_is==EXPR_RVAL || l2->expr_is==EXPR_LVAL || l2->expr_is==EXPR_HARDCODED_CONST)) {
        new_expr = (expr_t*)calloc(1,sizeof(expr_t));
        new_expr->parent = new_expr;
        new_expr->l1 = l1;
        new_expr->l2 = l2;
        //new_expr->datatype = SEM_BOOLEAN; //decide later
        new_expr->expr_is = EXPR_RVAL;
        new_expr->op = op;
        new_expr->ival = 0;
        new_expr->fval = 0;
        new_expr->cval = 0;

        l1->parent = new_expr;
        l2->parent = new_expr;

        if (!TYPE_IS_ARITHMETIC(l1->datatype) /*&& l1->expr_is!=EXPR_LVAL*/) {
            sprintf(str_err,"doing math with '%s' value",l1->datatype->name);
            yywarning(str_err);
        }

        if (!TYPE_IS_ARITHMETIC(l2->datatype) /*&& l2->expr_is!=EXPR_LVAL*/) {
            sprintf(str_err,"doing math with '%s' value",l2->datatype->name);
            yywarning(str_err);
        }
        //continue as normal

        if (l1->datatype->is!=l2->datatype->is) {
            //if we have at least one real, convert the other expr to real too
            if (l1->datatype->is==TYPE_REAL) {
                if (l2->expr_is==EXPR_HARDCODED_CONST) { l2->datatype = SEM_REAL; }
                else { l2->convert_to = SEM_REAL; }
                new_expr->datatype = SEM_REAL;
            } else if (l2->datatype->is==TYPE_REAL) {
                if (l1->expr_is==EXPR_HARDCODED_CONST) { l1->datatype = SEM_REAL; }
                else { l1->convert_to = SEM_REAL; }
                new_expr->datatype = SEM_REAL;
            } else {
                //both are of datatype integer
                new_expr->datatype = SEM_INTEGER;
            }
        } else if (l1->datatype->is==TYPE_REAL) {
            //both are real
            new_expr->datatype = SEM_REAL;
        } else {
            //both are not real, do it integer
            //here, if we are doing math with non arithmetic datatypes, consider them as integer
            new_expr->datatype = SEM_INTEGER;
        }

        //correct the datatype if we have RELOP operator
        switch (op) {
        case OP_PLUS:
        case OP_MINUS:
        case OP_MULT:
            //we already decided
            break;
        default:
            new_expr->datatype = SEM_BOOLEAN;
        }

        //simplify hardcoded operators for dependence analysis later
        if (op==OP_MINUS && l2->expr_is==EXPR_HARDCODED_CONST) {
            new_expr->expr_is = OP_PLUS;
            new_expr->ival = -new_expr->ival;
            new_expr->fval = -new_expr->fval;
            new_expr->cval = -new_expr->cval;
        }

	//optimization
        new_expr = expr_simplify_hardcoded(new_expr,l1,op,l2);
    }

    return new_expr;
}

expr_t *expr_inop(expr_t *l1,op_t op,expr_t *l2) {
    if (!l1 || !l2) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("null expression in expr_inop (debugging info)");
#endif
        return expr_from_hardcoded_boolean(0);
    }

    if (l1->expr_is==EXPR_LOST || l2->expr_is==EXPR_LOST) {
        return expr_from_hardcoded_boolean(0);
    }

    if (l2->expr_is==EXPR_SET || (l2->expr_is==EXPR_LVAL && l2->datatype->is==TYPE_SET)) {
        //it is not necessary for a `set` datatype to be declared previously, we just want a boolean result
        if (l2->expr_is==EXPR_LVAL) {
            if (l1->datatype!=l2->datatype->def_datatype) { //for the set datatype see expr_from_setexpression() from expr_toolbox.c
                yyerror("`in` operator: set expression and element must have the same type of data");
                return expr_from_hardcoded_boolean(0);
            }
        }
        else {
            if (l1->datatype!=l2->datatype) { //for the set datatype see expr_from_setexpression() from expressions.c
                yyerror("`in` operator: set expression and element must have the same type of data");
                return expr_from_hardcoded_boolean(0);
            }
        }
    }
    else if (l2->expr_is==EXPR_NULL_SET) {
        //the null set takes the type of l1 expression
        //#warning "always return FALSE when checking for existance in a NULL (empty) set"
        return expr_from_hardcoded_boolean(0);
    }
    else {
        yyerror("expected expression of `set` type after `in` operator");
        return expr_from_hardcoded_boolean(0);
    }

    return expr_distribute_inop_to_set(l1,l2);
}

expr_t *expr_orop_andop_notop(expr_t *l1,op_t op,expr_t *l2) {
    //operand OP_NOT uses the l2 expression
    expr_t *new_expr;

    if (!l2 || (!l1 && op!=OP_NOT)) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("null expression in expr_orop (debugging info)");
#endif
        return expr_from_hardcoded_boolean(0);
    }

    if ((op!=OP_NOT && l1->expr_is==EXPR_LOST) || l2->expr_is==EXPR_LOST) {
        return expr_from_hardcoded_boolean(0);
    }

    if ((op!=OP_NOT && l1->datatype->is!=TYPE_BOOLEAN) || l2->datatype->is!=TYPE_BOOLEAN) {
        sprintf(str_err,"`%s' operator applies only to booleans",op_literal(op));
        yyerror(str_err);
        return expr_from_hardcoded_boolean(0);
    }

    //if l2 is alerady OP_NOT return the initial expr;
    if (op==OP_NOT && l2->op==OP_NOT) {
        //reminder: hardcoded expressions have op==OP_IGNORE
        //there is no case for l2 to be hardcoded in here
        return l2->l2;
    }

    switch (op) {
    case OP_AND:
        //both hardcoded
        if (l1->expr_is==EXPR_HARDCODED_CONST && l2->expr_is==EXPR_HARDCODED_CONST)
            return expr_from_hardcoded_boolean((l1->cval && l2->cval)?1:0);  //and

        if (l1->expr_is==EXPR_HARDCODED_CONST)
            return (l1->cval) ? l2 : l1;

        if (l2->expr_is==EXPR_HARDCODED_CONST)
            return (l2->cval) ? l1 : l2;

        break;
    case OP_OR:
        //both hardcoded
        if (l1->expr_is==EXPR_HARDCODED_CONST && l2->expr_is==EXPR_HARDCODED_CONST)
            return expr_from_hardcoded_boolean((l1->cval || l2->cval)?1:0);  //or

        if (l1->expr_is==EXPR_HARDCODED_CONST)
            return (l1->cval) ? l1 : l2;

        if (l2->expr_is==EXPR_HARDCODED_CONST)
            return (l2->cval) ? l2 : l1;

        break;
    case OP_NOT:
        if (l2->expr_is==EXPR_HARDCODED_CONST)
            return expr_from_hardcoded_boolean((l2->cval)?0:1); //not
        break;
    }

    //no hardcoded value

    new_expr = (expr_t*)calloc(1,sizeof(expr_t));
    new_expr->parent = new_expr;
    new_expr->datatype = SEM_BOOLEAN;
    new_expr->expr_is = EXPR_RVAL;
    new_expr->op = op;
    new_expr->l1 = l1;
    new_expr->l2 = l2;
    l2->parent = new_expr;

    if (op!=OP_NOT) {
        //in this case l1 is NULL
        l1->parent = new_expr;
    }

    return new_expr;
}

expr_t *expr_muldivandop(expr_t *l1,op_t op,expr_t *l2) {
    expr_t *new_expr;

    if (!l1 || !l2) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("null expression in expr_muldivandop (debugging info)");
#endif
        return expr_from_hardcoded_int(0);
    }

    if (op==OP_MULT) {
        return expr_relop_equ_addop_mult(l1,op,l2);
    }
    else if (op==OP_AND) {
        return expr_orop_andop_notop(l1,op,l2);
    }

    if (l1->expr_is==EXPR_LOST || l2->expr_is==EXPR_LOST) {
        switch (op) {
        case OP_DIV:
        case OP_MOD:
            return expr_from_hardcoded_int(0);
        case OP_RDIV:
            return expr_from_hardcoded_real(0);
        default:
            die("UNEXPECTED_ERROR: 89-1");
        }
    }

    if (!TYPE_IS_ARITHMETIC(l1->datatype) || !TYPE_IS_ARITHMETIC(l2->datatype)) {
        sprintf(str_err,"'%s' operator applies only to integers and reals",op_literal(op));
        yyerror(str_err);
    }

    if (l2->expr_is==EXPR_HARDCODED_CONST && l2->ival==0) {
        yyerror("division by zero");
        return expr_from_hardcoded_int(0);
    }

    if (l1->expr_is==EXPR_HARDCODED_CONST && l2->expr_is==EXPR_HARDCODED_CONST) {	//optimization
        switch (op) {
        case OP_RDIV: return expr_from_hardcoded_real(l1->fval / l2->fval);
        case OP_DIV:  return expr_from_hardcoded_int(l1->ival / l2->ival);
        case OP_MOD:  return expr_from_hardcoded_int(l1->ival % l2->ival);
        default:
            die("UNEXPECTED_ERROR: 89-2-1");
        }
    }

    switch (op) {
    case OP_RDIV:
        //the `/` op always returns real
        if (l1->expr_is==EXPR_HARDCODED_CONST) { l1->datatype = SEM_REAL; }
        else if (l1->datatype->is!=TYPE_REAL) { l1->convert_to = SEM_REAL; }

        if (l2->expr_is==EXPR_HARDCODED_CONST) { l2->datatype = SEM_REAL; }
        else if (l2->datatype->is!=TYPE_REAL) { l2->convert_to = SEM_REAL; }

        new_expr = (expr_t*)calloc(1,sizeof(expr_t));
        new_expr->parent = new_expr;
        new_expr->l1 = l1;
        new_expr->l2 = l2;
        new_expr->datatype = SEM_REAL;
        new_expr->expr_is = EXPR_RVAL;
        new_expr->op = op;

        l1->parent = new_expr;
        l2->parent = new_expr;

        return new_expr;
    case OP_DIV:
    case OP_MOD:
        //applies only on integer expressions
        if (l1->datatype->is!=TYPE_INT || l2->datatype->is!=TYPE_INT) {
            yywarning("`div`, 'mod' apply only to integers");
#warning this should be an error or a warning?
            //return expr_from_hardcoded_int(0);
        }

        if (l1->expr_is==EXPR_HARDCODED_CONST) { l1->datatype = SEM_INTEGER; }
        else if (l1->datatype->is!=TYPE_INT) { l1->convert_to = SEM_INTEGER; }

        if (l2->expr_is==EXPR_HARDCODED_CONST) { l2->datatype = SEM_INTEGER; }
        else if (l2->datatype->is!=TYPE_INT) { l2->convert_to = SEM_INTEGER; }

        new_expr = (expr_t*)calloc(1,sizeof(expr_t));
        new_expr->parent = new_expr;
        new_expr->l1 = l1;
        new_expr->l2 = l2;
        new_expr->datatype = SEM_INTEGER;
        new_expr->expr_is = EXPR_RVAL;
        new_expr->op = op;

        l1->parent = new_expr;
        l2->parent = new_expr;

        return new_expr;
    default:
        die("UNEXPECTED_ERROR: 89-4");
        return NULL; //keep the compiler happy
    }
}

expr_t *expr_sign(op_t op,expr_t *l) {
    expr_t *new_expr;

    if (!l) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("null expression in expr_sign (debugging info)");
#endif
        return expr_from_hardcoded_int(0);
    }

    if (l->expr_is==EXPR_LOST) {
        return expr_from_hardcoded_int(0);
    }

    if (!TYPE_IS_ARITHMETIC(l->datatype)) {
        yyerror("only integers and reals can be signed");
        return expr_from_hardcoded_int(0);
    }

    if (l->op==OP_SIGN) {
        yyerror("2 sign operators in a row are not allowed, ignoring the second sign");
        return l;
    }

    if (op==OP_MINUS) {
        if (l->expr_is==EXPR_HARDCODED_CONST) { //optimization
            switch (l->datatype->is) {
            case TYPE_INT:   return expr_from_hardcoded_int(-l->ival);
            case TYPE_REAL:  return expr_from_hardcoded_real(-l->fval);
            default:
                //should never reach here
                die("UNEXPECTED_ERROR: in sign expression");
            }
        }

        new_expr = (expr_t*)calloc(1,sizeof(expr_t));
        new_expr->parent = new_expr;

        if (l->expr_is==EXPR_SET || l->expr_is==EXPR_NULL_SET) {
            new_expr->l1 = expr_from_setexpression(NULL);
        } else if (l->datatype->is==TYPE_REAL) {
            new_expr->l1 = expr_from_hardcoded_real(0);
        } else {
            new_expr->l1 = expr_from_hardcoded_int(0);
        }

        new_expr->l2 = l;
        new_expr->datatype = l->datatype;
        new_expr->expr_is = EXPR_RVAL;
        //new_expr->op = OP_SIGN;
        new_expr->op = OP_MINUS;
        l->parent = new_expr;
        return new_expr;

    }

    //ignore OP_PLUS

    return l;
}

int expr_lval_with_comp_datatype(expr_t *l) {
    var_t *v;

    if (l->expr_is==EXPR_LVAL && TYPE_IS_COMPOSITE(l->datatype)) {
        v = l->var;
        sprintf(str_err,"doing math with '%s' of composite datatype '%s'",v->name,v->datatype->name);
        yyerror(str_err);
        l->expr_is = EXPR_LOST;
        l->var = lost_var_reference();
        l->datatype = l->var->datatype;
        return 1;
    }

    return 0;
}

expr_t *expr_simplify_hardcoded(expr_t *new_expr, expr_t *l1, op_t op, expr_t *l2) {
    if (l1->expr_is==EXPR_HARDCODED_CONST && l2->expr_is==EXPR_HARDCODED_CONST) {
        //both datatypes are the same
        new_expr->expr_is = EXPR_HARDCODED_CONST;
        switch (op) {
        case OP_PLUS:
            new_expr->ival = l1->ival + l2->ival;	//if integer
            new_expr->fval = l1->fval + l2->fval;	//if real
            new_expr->cval = l1->cval + l2->cval;	//if char, boolean
            break;
        case OP_MINUS:
            new_expr->ival = l1->ival - l2->ival;	//if integer
            new_expr->fval = l1->fval - l2->fval;	//if real
            new_expr->cval = l1->cval - l2->cval;	//if char, boolean
            break;
        case OP_MULT:
            new_expr->ival = l1->ival * l2->ival;	//if integer
            new_expr->fval = l1->fval * l2->fval;	//if real
            new_expr->cval = l1->cval * l2->cval;	//if char, boolean
            break;
        case RELOP_B:
            if (l1->fval > l2->fval || l1->ival > l2->ival) {
                new_expr->ival = 1;
                new_expr->fval = 1;
                new_expr->cval = 1;
            }
            break;
        case RELOP_BE:
            if (l1->fval >= l2->fval || l1->ival >= l2->ival) {
                new_expr->ival = 1;
                new_expr->fval = 1;
                new_expr->cval = 1;
            }
            break;
        case RELOP_L:
            if (l1->fval < l2->fval || l1->ival < l2->ival) {
                new_expr->ival = 1;
                new_expr->fval = 1;
                new_expr->cval = 1;
            }
            break;
        case RELOP_LE:
            if (l1->fval <= l2->fval || l1->ival <= l2->ival) {
                new_expr->ival = 1;
                new_expr->fval = 1;
                new_expr->cval = 1;
            }
            break;
        case RELOP_NE:
            if (l1->fval != l2->fval || l1->ival != l2->ival) {
                new_expr->ival = 1;
                new_expr->fval = 1;
                new_expr->cval = 1;
            }
            break;
        case RELOP_EQU:
            if (l1->fval == l2->fval || l1->ival == l2->ival) {
                new_expr->ival = 1;
                new_expr->fval = 1;
                new_expr->cval = 1;
            }
            break;
        default:
            die("UNEXPECTED ERROR: expressions.c : bad operator");
            break;
        }
        //we don't need the previous hardcoded values any more
        new_expr->l1 = NULL;
        new_expr->l2 = NULL;

        return new_expr;
    }

    if (l1->expr_is==EXPR_HARDCODED_CONST) {
        //both datatypes are the same
        switch (op) {
        case OP_PLUS:
            if (l1->ival==0 || l1->fval==0) {
                free(new_expr);
                return l2;
            }
            break;
        case OP_MINUS:
            return new_expr;

            break;
        case OP_MULT:
            if (l1->ival==0 || l1->fval==0) {
                free(new_expr);
                return l1;
            }

            if (l1->ival==1 || l1->fval==1) {
                free(new_expr);
                return l2;
            }

            break;
        case RELOP_B:
        case RELOP_BE:
        case RELOP_L:
        case RELOP_LE:
        case RELOP_NE:
        case RELOP_EQU:
            break;
        default:
            die("UNEXPECTED ERROR: expressions.c : bad operator");
            break;
        }

        return new_expr;
    }

    if (l2->expr_is==EXPR_HARDCODED_CONST) {
        //both datatypes are the same
        switch (op) {
        case OP_PLUS:
        case OP_MINUS:
            if (l2->ival==0 || l2->fval==0) {
                free(new_expr);
                return l1;
            }
            break;
        case OP_MULT:
            if (l2->ival==0 || l2->fval==0) {
                free(new_expr);
                return l2;
            }

            if (l2->ival==1 || l2->fval==1) {
                free(new_expr);
                return l1;
            }

            break;
        case RELOP_B:
        case RELOP_BE:
        case RELOP_L:
        case RELOP_LE:
        case RELOP_NE:
        case RELOP_EQU:
            break;
        default:
            die("UNEXPECTED ERROR: expressions.c : bad operator");
            break;
        }

        return new_expr;
    }

    //cannot perform optimizations of this type
    return new_expr;
}
