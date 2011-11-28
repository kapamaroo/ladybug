#include <stdio.h>
#include <stdlib.h>

#include "build_flags.h"
#include "expr_toolbox.h"
#include "expressions.h"
#include "symbol_table.h"
#include "mem_reg.h"
#include "err_buff.h"

expr_t *expr_from_variable(var_t *v) {
    expr_t *l;

    if (!v) {
        //error in variable declaration
		yyerror("INTERNAL_ERROR: 44");
		exit(EXIT_FAILURE);
    }

	if (v->id_is==ID_CONST) {
		if (v->datatype->is==TYPE_INT || v->datatype->is==TYPE_ENUM) {
			l = expr_from_hardcoded_int(v->ival);
			l->datatype = v->datatype; //if enum, set the enumeration type
		}
		else if (v->datatype->is==TYPE_REAL) {
			l = expr_from_hardcoded_real(v->ival);
		}
		else if (v->datatype->is==TYPE_CHAR) {
			l = expr_from_hardcoded_char(v->ival);
		}
		else if (v->datatype->is==TYPE_BOOLEAN) {
			l = expr_from_hardcoded_boolean(v->ival);
		}
		else {
			yyerror("UNEXPECTED_ERROR: 38");
			exit(EXIT_FAILURE);
		}
		return l;
	}

	l = (expr_t*)malloc(sizeof(expr_t));
	l->parent = l;
	l->l1 = NULL;
	l->l2 = NULL;
	l->op = OP_IGNORE;
	l->expr_is = EXPR_LVAL; //ID_VAR or ID_VAR_PARAM or ID_VAR_GUARDED or ID_RETURN or ID_CONST or ID_LOST
	l->ival = v->ival; //for int ID_CONST
	l->fval = v->fval; //for real ID_CONST
	l->cval = v->cval; //for char ID_CONST
	l->cstr = NULL;
	l->var = v;
	l->datatype = v->datatype;
	l->convert_to = NULL;

	if (v->id_is==ID_VAR || v->id_is==ID_VAR_PTR) {
		if (v->datatype->is==TYPE_SUBSET) {
			l->datatype = v->datatype->def_datatype; //integer,char,boolean, or enumeration
		}
		//arrays and record types are allowed only for asignments
	}
	else if (v->id_is==ID_LOST) {
		l->expr_is = EXPR_LOST;
	}

	return l;
}

expr_t *expr_from_STRING(char *id) {
    //cannot have const char as string because of the """
    expr_t *new_expr;
	var_t *dummy_var;

	dummy_var = (var_t*)malloc(sizeof(var_t));
	dummy_var->id_is = ID_STRING;
	dummy_var->datatype = SEM_CHAR;
	dummy_var->cstr = id;
	dummy_var->scope = get_current_scope();
	dummy_var->Lvalue = mem_allocate_string(id);

    new_expr = (expr_t*)malloc(sizeof(expr_t));
    new_expr->parent = new_expr;
    new_expr->l1 = NULL;
    new_expr->l2 = NULL;
	new_expr->op = OP_IGNORE;
    new_expr->expr_is = EXPR_STRING;
    new_expr->datatype = void_datatype; //FIXME
    new_expr->cstr = id; //no sdtrdup here, flex allocates memory
	new_expr->var = dummy_var;
    return new_expr;
}

expr_t *expr_from_hardcoded_int(int value) {
	expr_t *new_expr;

	new_expr = (expr_t*)malloc(sizeof(expr_t));
	new_expr->parent = new_expr;
	new_expr->l1 = NULL;
	new_expr->l2 = NULL;
	new_expr->op = OP_IGNORE;
	new_expr->expr_is = EXPR_HARDCODED_CONST;
	new_expr->datatype = SEM_INTEGER;
	new_expr->ival = value;
	new_expr->fval = value;
	new_expr->cval = value;

	return new_expr;
}

expr_t *expr_from_lost_int(int value) {
	expr_t *new_expr;

	new_expr = expr_from_hardcoded_int(value);
	new_expr->expr_is = EXPR_LOST;

	return new_expr;
}

expr_t *expr_from_signed_hardcoded_int(op_t op,int value) {
	if (op==OP_MINUS) {
		return expr_from_hardcoded_int(-value);
	}
	return expr_from_hardcoded_int(value);
}

expr_t *expr_from_hardcoded_real(float value) {
	expr_t *new_expr;

	new_expr = (expr_t*)malloc(sizeof(expr_t));
	new_expr->parent = new_expr;
	new_expr->l1 = NULL;
	new_expr->l2 = NULL;
	new_expr->op = OP_IGNORE;
	new_expr->expr_is = EXPR_HARDCODED_CONST;
	new_expr->datatype = SEM_REAL;
	new_expr->fval = value;
	new_expr->ival = value;
	new_expr->cval = value;

	return new_expr;
}

expr_t *expr_from_hardcoded_boolean(int value) {
	expr_t *new_expr;

	new_expr = (expr_t*)malloc(sizeof(expr_t));
	new_expr->parent = new_expr;
	new_expr->l1 = NULL;
	new_expr->l2 = NULL;
	new_expr->op = OP_IGNORE;
	new_expr->expr_is = EXPR_HARDCODED_CONST;
	new_expr->datatype = SEM_BOOLEAN;
	new_expr->ival = value;
	new_expr->fval = value;
	new_expr->cval = value;

	return new_expr;
}

expr_t *expr_from_lost_boolean(int value) {
	expr_t *new_expr;

	new_expr = expr_from_hardcoded_boolean(value);
	new_expr->expr_is = EXPR_LOST;

	return new_expr;
}

expr_t *expr_from_hardcoded_char(char value) {
	expr_t *new_expr;

	new_expr = (expr_t*)malloc(sizeof(expr_t));
	new_expr->parent = new_expr;
	new_expr->l1 = NULL;
	new_expr->l2 = NULL;
	new_expr->op = OP_IGNORE;
	new_expr->expr_is = EXPR_HARDCODED_CONST;
	new_expr->datatype = SEM_CHAR;
	new_expr->ival = value;
	new_expr->fval = value;
	new_expr->cval = value;

	return new_expr;
}

expr_t *expr_from_setexpression(elexpr_list_t *list) {
    //assign values to a set type
    //invalid elexressions are being ingnored and if all elexpressions are invalid, the null set is created.
    //These actions take place just to continue parsing.
    //In both cases an error is printed
    expr_t *new_expr;

    new_expr = (expr_t*)malloc(sizeof(expr_t));
    new_expr->parent = new_expr;
	new_expr->expr_is = EXPR_NULL_SET;
    new_expr->l1 = NULL;
    new_expr->l2 = NULL;
    new_expr->op = OP_IGNORE;
    new_expr->elexpr_list = NULL;
	new_expr->datatype = SEM_CHAR; //default type of set


    if (list && list->elexpr_list_usage==EXPR_SET) {
        new_expr->expr_is = EXPR_SET;
        //all expressions in elexpr_list are of the same type, just pick one
        new_expr->datatype = list->elexpr_list_datatype;
        new_expr->elexpr_list = list;
    }

    return new_expr;
}

expr_t *expr_distribute_inop_to_set(expr_t *el,expr_t *expr_set) {
	//applies the 'in' operator to each set separately
	//play with the logical values, not with the sets, it's easier i think
	//returns a boolean expression
	expr_t *new_cond;
	expr_t *new_inop;
	expr_t *left_cond;
	expr_t *right_cond;
	expr_t *new_not_cond;

	op_t op;

	if (!el || !expr_set) {
		return expr_from_hardcoded_boolean(0);
	}

	op = expr_set->op;
	switch (op) {
	case OP_IGNORE:
		new_inop = (expr_t*)malloc(sizeof(expr_t));
		new_inop->parent = new_inop;
		new_inop->datatype = SEM_BOOLEAN;
		new_inop->expr_is = EXPR_RVAL;
		new_inop->op = RELOP_IN;
		new_inop->l1 = el;
		new_inop->l2 = expr_set;
		el->parent = new_inop;
		expr_set->parent = new_inop;
		return new_inop;
	case OP_MULT:	//becomes OP_AND
	case OP_PLUS:	//becomes OP_OR
		new_cond = (expr_t*)malloc(sizeof(expr_t));
		new_cond->parent = new_cond;
		new_cond->datatype = SEM_BOOLEAN;
		new_cond->expr_is = EXPR_RVAL;

		left_cond = expr_distribute_inop_to_set(el,expr_set->l1);
		right_cond = expr_distribute_inop_to_set(el,expr_set->l2);

		left_cond->parent = new_cond;
		right_cond->parent = new_cond;

		new_cond->op = (op==OP_MULT)?OP_AND:OP_OR;
		new_cond->l1 = left_cond;
		new_cond->l2 = right_cond;
		return new_cond;
	case OP_MINUS:	//becomes OP_AND (OP_NOT)
		left_cond = expr_distribute_inop_to_set(el,expr_set->l1);
		right_cond = expr_distribute_inop_to_set(el,expr_set->l2);

		new_cond = (expr_t*)malloc(sizeof(expr_t));
		new_cond->parent = new_cond;
		new_cond->datatype = SEM_BOOLEAN;
		new_cond->expr_is = EXPR_RVAL;

		new_not_cond = (expr_t*)malloc(sizeof(expr_t));
		new_not_cond->parent = new_not_cond;
		new_not_cond->datatype = SEM_BOOLEAN;
		new_not_cond->expr_is = EXPR_RVAL;

		new_not_cond->op = OP_NOT;
		new_not_cond->l1 = NULL;
		new_not_cond->l2 = right_cond;
		right_cond->parent = new_not_cond;

		new_cond->op = OP_AND;
		new_cond->l1 = left_cond;
		new_cond->l2 = new_not_cond;
		left_cond->parent = new_cond;
		new_not_cond->parent = new_cond;

		return new_cond;
	default:
		printf("UNEXPECTED_ERROR: 51-2");
		exit(EXIT_FAILURE);
	}
}

char *op_literal(op_t op) {
	switch (op) {
	case OP_IGNORE:
		return "__op_IGNORE__";
	case RELOP_B:	// '>'
		return ">";
	case RELOP_BE:	// '>='
		return ">=";
	case RELOP_L:	// '<'
    	return "<";
	case RELOP_LE:	// '<='
    	return "<=";
	case RELOP_NE:	// '<>'
    	return "<>";
	case RELOP_EQU:	// '='
		return "=";
	case RELOP_IN:		// 'in'
    	return "in";
	case OP_SIGN: 	//dummy operator, to determine when the the OP_PLUS, OP_MINUS are used as sign
    	return "op_SIGN";
	case OP_PLUS:	// '+'
    	return "+";
	case OP_MINUS:	// '-'
    	return "-";
	case OP_MULT:	// '*'
    	return "*";
	case OP_RDIV:	// '/'
    	return "/";
	case OP_DIV:		// 'div'
    	return "div";
	case OP_MOD:		// 'mod'
    	return "mod";
	case OP_AND:		// 'and'
    	return "and";
	case OP_OR:		// 'or'
    	return "or";
	case OP_NOT:		// 'not'
		return "not";
	default:
		yyerror("UNEXPECTED_ERROR: 04");
		exit(EXIT_FAILURE);
	}
}

int check_if_boolean(expr_t *l) {
    if (!l) {
		yyerror("UNEXPECTED_ERROR: 03");
		exit(EXIT_FAILURE);
	}
	else if (l->datatype->is==TYPE_BOOLEAN) {
        return 1;
    }
    yyerror("ERROR: a flow control expression must be boolean");
    return 0;
}

expr_t * normalize_expr_set(expr_t* expr_set){
	expr_t *new_expr;
	expr_t *tmp1;
	expr_t *tmp2;
	expr_t *tmp3;
	expr_t *tmp4;
	expr_t *expr1;
	expr_t *expr2;

	expr_t *l1;
	op_t op;
	expr_t *l2;

	l1 = expr_set->l1;
	op = expr_set->op;
	l2 = expr_set->l2;

	if (op==OP_IGNORE || op==OP_PLUS || op==OP_MINUS) {
		return expr_set;
	}
	else if (op==OP_MULT) {
		if ((l1->op==OP_MULT && l2->op==OP_MULT) || (l1->op==OP_IGNORE && l2->op==OP_IGNORE)) {
			return expr_set;
		}
		else if ((l1->op!=OP_MULT && l1->op!=OP_IGNORE) && (l2->op!=OP_MULT && l2->op!=OP_IGNORE)) {
			tmp1 = expr_muldivandop(l1->l1,op,l2->l1);
			tmp2 = expr_muldivandop(l1->l1,op,l2->l2);
			tmp3 = expr_muldivandop(l1->l2,op,l2->l1);
			tmp4 = expr_muldivandop(l1->l2,op,l2->l2);
			expr1 = expr_relop_equ_addop_mult(tmp1,l2->op,tmp2);
			expr2 = expr_relop_equ_addop_mult(tmp3,l2->op,tmp4);
			new_expr = expr_relop_equ_addop_mult(expr1,l1->op,expr2);
		}
		else if (l1->op!=OP_MULT && l1->op!=OP_IGNORE) {
			expr2 = normalize_expr_set(l2);
			tmp1 = expr_muldivandop(l1->l1,op,expr2);
			tmp2 = expr_muldivandop(l1->l2,op,expr2);
			new_expr = expr_relop_equ_addop_mult(tmp1,l1->op,tmp2);
		}
		else {
			expr1 = normalize_expr_set(l1);
			tmp1 = expr_muldivandop(expr1,op,l2->l1);
			tmp2 = expr_muldivandop(expr1,op,l2->l2);
			new_expr = expr_relop_equ_addop_mult(tmp1,l2->op,tmp2);
		}
		return new_expr;
	}
	yyerror("UNEXPECTED_ERROR: 85-1");
	exit(EXIT_FAILURE);
}
