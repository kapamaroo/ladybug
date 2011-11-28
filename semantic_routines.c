#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "semantic_routines.h"
#include "expressions.h"
#include "expr_toolbox.h"
#include "symbol_table.h"
#include "mem_reg.h"
#include "ir.h"
#include "ir_toolbox.h"
#include "err_buff.h"

void init_semantics() {

}

dim_t *make_dim_bounds(expr_t *l1,expr_t *l2) {
//both expressions should have the same type and (l1 <= l2)
//maybe we need one more parameter to identify if the check is for subset `limits` or for an array's dimension
    dim_t *new_dim;

    new_dim = (dim_t*)malloc(sizeof(dim_t));
	new_dim->first = 0;
    new_dim->range = 1;

    if (!TYPE_IS_SCALAR(l1->datatype)) {
        yyerror("ERROR: nonscalar type of left dim bound");
    }
    else if (!TYPE_IS_SCALAR(l2->datatype)) {
        yyerror("ERROR: type of right dim bound not scalar");
    }
	else if (l1->expr_is!=EXPR_HARDCODED_CONST || l2->expr_is!=EXPR_HARDCODED_CONST) {
		yyerror("ERROR: dim bounds MUST be constants");
	}
    else if (l2->ival - l1->ival + 1<=0 || l2->ival - l1->ival + 1>MAX_FIELDS) {
        yyerror("ERROR: dimension bounds are incorrect");
    }
	else {
		new_dim->first = l1->ival;
		new_dim->range = l2->ival - l1->ival + 1;
	}
    return new_dim;
}

dim_t *make_dim_bound_from_id(char *id) {
    dim_t *new_dim;
    sem_t *sem_1;

    new_dim = (dim_t*)malloc(sizeof(dim_t));
	new_dim->first = 0;
    new_dim->range = 1;

    sem_1 = sm_find(id);
    if (sem_1) {
		if (sem_1->id_is==ID_TYPEDEF && (sem_1->comp->is==TYPE_ENUM || sem_1->comp->is==TYPE_SUBSET)) {
            if (strcmp(sem_1->comp->data_name,id)==0) {
                //the range is the whole enumeration or subset
                new_dim->first = sem_1->comp->enum_num[0];
                new_dim->range = sem_1->comp->enum_num[sem_1->comp->field_num-1];
            }
            else {
                //id is an element name of enum or subset type
                //which means the size of dimension is 1
                yyerror("ERROR: id name is an element of a enum or subset type, the size of dimension is 1");
            }
        }
        else {
            yyerror("ERROR: `limit` id type mismatch");
        }
    }
    else {
        if (!sm_find_lost_symbol(id)) {
            sm_insert_lost_symbol(id);
            sprintf(str_err,"ERROR: undeclared symbol '%s'",id);
            yyerror(str_err);
        }
    }
    return new_dim;
}

var_t *refference_to_array_element(var_t *v, expr_list_t *list) {
    //reference to array
    expr_t *relative_offset_expr;
	expr_t *final_offset_expr;
    expr_t *cond_expr;
	mem_t *new_mem;
    var_t *new_var;

    if (!list) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("UNEXPECTED_ERROR: null expr_list for array refference (debugging info)");
#endif
        exit(EXIT_FAILURE);
    }

    if (v) {
        if (v->id_is==ID_VAR || v->id_is==ID_VAR_PTR) {
            if (v->datatype->is==TYPE_ARRAY) {
                if (valid_expr_list_for_array_reference(v->datatype,list)) {
                    relative_offset_expr = make_array_refference(list,v->datatype);
                    cond_expr = make_array_bound_check(list,v->datatype);
                }
                else {
                    relative_offset_expr = expr_from_hardcoded_int(0);
                    cond_expr = expr_from_hardcoded_boolean(0);
                }
                final_offset_expr = expr_relop_equ_addop_mult(v->Lvalue->offset_expr,OP_PLUS,relative_offset_expr);

				//we start from the variable's mem position and we add the offset from there
				new_mem = (mem_t*)malloc(sizeof(mem_t));
				new_mem->direct_register_number = v->Lvalue->direct_register_number;
				new_mem->segment = v->Lvalue->segment;
				new_mem->seg_offset = v->Lvalue->seg_offset;
				new_mem->offset_expr = final_offset_expr;
				new_mem->content_type = v->Lvalue->content_type;
				new_mem->size = v->datatype->def_datatype->memsize;

                new_var = (var_t*)malloc(sizeof(var_t));
                new_var->id_is = ID_VAR;
                new_var->datatype = v->datatype->def_datatype;
                new_var->name = v->name;
                new_var->scope = v->scope;
                new_var->Lvalue = new_mem;
				new_var->cond_assign = cond_expr;
                return new_var;
            }
            else {
                sprintf(str_err,"ERROR: variable '%s' is not an array",v->name);
				yyerror(str_err);
				return lost_var_reference(); //avoid unreal error messages
            }
        }
		else if (v->id_is==ID_LOST) {
			return v; //avoid unreal error messages
		}
        else {
            sprintf(str_err,"ERROR: id '%s' is not a variable",v->name);
            yyerror(str_err);
			return lost_var_reference();
        }
    }
    else {
		yyerror("UNEXPECTED_ERROR: 42");
		exit(EXIT_FAILURE);
    }
}

int valid_expr_list_for_array_reference(data_t *data,expr_list_t *list) {
    int i;
    int index;
    expr_t *l;

    for (i=0;i<data->field_num;i++) {
        l = list->expr_list[i];
        if (TYPE_IS_SCALAR(l->datatype)) {
            switch (l->expr_is) {
            case EXPR_LOST:
                return 0;
            case EXPR_HARDCODED_CONST:
                index = l->ival - data->dim[i]->first;
                if (index<0 || index > data->dim[i]->range) {
                    sprintf(str_err,"ERROR: reference to the %d dimension of array, out of bounds",i);
                    yyerror(str_err);
                    return 0;
                }
                break; //static check here, do not generate dynamic checks here, we already know the answer
            case EXPR_LVAL:
            case EXPR_RVAL:
                break;
            default:
                //should never reach here
                printf("UNEXPECTED ERROR 30\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (l->datatype==void_datatype) {
            //EXPR_LOST and EXPR_STRING have void_datatype
            return 0;
        }
        else {
            sprintf(str_err,"ERROR: reference to the %d dimension of array with nonscalar datatype '%s'",i,l->datatype->data_name);
            yyerror(str_err);
            return 0;
        }
    }
    return 1;
}

expr_t *make_array_refference(expr_list_t *list,data_t *data) {
    int i;
	expr_t *l;
    expr_t *dim_size;
    expr_t *dim_index;
    expr_t *dim_offset;
    expr_t *total_offset;
    expr_t *datadef_size;

    //start with zero offset
    total_offset = expr_from_hardcoded_int(0);

    //make expression of datadef_type size
    datadef_size = expr_from_hardcoded_int(data->def_datatype->memsize);
    for (i=0;i<data->field_num;i++) {
        l = list->expr_list[i];
        switch (l->expr_is) {
        case EXPR_HARDCODED_CONST:
        case EXPR_LVAL:
        case EXPR_RVAL:
            //calculate partial offset
            dim_index = l;
            dim_size = expr_from_hardcoded_int(data->dim[i]->relative_distance);
            dim_offset = expr_muldivandop(dim_size,OP_MULT,dim_index); //new offset for current dimension
            total_offset = expr_relop_equ_addop_mult(total_offset,OP_PLUS,dim_offset); //add the new offset with the previous
            break;
        default:
            //should never reach here
            printf("UNEXPECTED ERROR 30-30\n");
            exit(EXIT_FAILURE);
        }
        total_offset = expr_muldivandop(total_offset,OP_MULT,datadef_size); //multiply with sizeof datadef type
    }
    return total_offset;
}

expr_t *make_array_bound_check(expr_list_t *list,data_t *data) {
    int i;
	int index;
	expr_t *l;
	expr_t *left_bound;
	expr_t *right_bound;
	expr_t *left_cond;
	expr_t *right_cond;
	expr_t *tmp_cond;
	expr_t *total_cond;

    total_cond = expr_from_hardcoded_boolean(1); //TRUE

    if (data->field_num==MAX_EXPR_LIST-list->expr_list_empty) {
        //make expression of datadef_type size
        for (i=0;i<data->field_num;i++) {
            l = list->expr_list[i];
            switch (l->expr_is) {
            case EXPR_HARDCODED_CONST:
                index = l->ival - data->dim[i]->first;
                if (index<0 || index > data->dim[i]->range) {
                    sprintf(str_err,"ERROR: reference to the %d dimension of array, out of bounds",i);
                    yyerror(str_err);
                    return expr_from_hardcoded_boolean(0);
                }
                break; //static check here, do not generate dynamic checks here, we already know the answer
            case EXPR_LVAL:
            case EXPR_RVAL:
                left_bound = expr_from_hardcoded_int(data->dim[i]->first); //the left (small) bound
                right_bound = expr_from_hardcoded_int(data->dim[i]->first + data->dim[i]->range - 1); //the right (big) bound
                left_cond = expr_relop_equ_addop_mult(left_bound,RELOP_LE,l);
                right_cond = expr_relop_equ_addop_mult(l,RELOP_LE,right_bound);
                tmp_cond = expr_orop_andop_notop(left_cond,OP_AND,right_cond);
                total_cond = expr_orop_andop_notop(total_cond,OP_AND,tmp_cond);
                break;
            default:
                //should never reach here
                printf("UNEXPECTED ERROR 30\n");
                exit(EXIT_FAILURE);
            }
        }
        return total_cond;
    }
    else if (data->field_num==list->all_expr_num) {
        //some expressions are invalid, but their number is correct, this avoids some unreal error messages afterwards
        return expr_from_hardcoded_boolean(0);
    }
    else {
        sprintf(str_err,"ERROR: different number of dimensions (%d) and dimension refferences (%d)",data->field_num, MAX_EXPR_LIST-list->expr_list_empty);
        yyerror(str_err);
        return expr_from_hardcoded_boolean(0);
    }
}

iter_t *make_iter_space(expr_t *l1,int step,expr_t *l3) {
    //#warning "check idt_type of expressions, control variable must be integer"
    //type cast the two expr if needed and if it's allowed

    iter_t *new_iter;

    if (!l1 || !l3) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("ERROR: NULL expression in make_iter_space() (debugging info)");
#endif
        return NULL;
    }

	if (l1->expr_is!=EXPR_LVAL && l1->expr_is!=EXPR_HARDCODED_CONST) {
		yyerror("ERROR: invalid expr for left bound in iter space");
		return NULL;
	}

	if (l1->expr_is==EXPR_LVAL && l1->var->datatype->is!=TYPE_INT && l1->var->datatype->def_datatype->is!=TYPE_INT) {
		yyerror("ERROR: left bound in iter space MUST be integer");
		return NULL;
	}

	if (l3->expr_is!=EXPR_LVAL && l3->expr_is!=EXPR_HARDCODED_CONST) {
		yyerror("ERROR: invalid expr for right bound in iter space");
		return NULL;
	}

	if (l3->expr_is==EXPR_LVAL && l3->var->datatype->is!=TYPE_INT && l3->var->datatype->def_datatype->is!=TYPE_INT) {
		yyerror("ERROR: right bound in iter space MUST be integer");
		return NULL;
	}

	new_iter = (iter_t*)malloc(sizeof(iter_t));
	new_iter->start = l1;
	new_iter->stop = l3;
	new_iter->step = expr_from_hardcoded_int(step);

	if (l1->expr_is==EXPR_HARDCODED_CONST && l3->expr_is==EXPR_HARDCODED_CONST &&
		new_iter->start->ival > new_iter->stop->ival) {
		yyerror("ERROR: unreachable 'for' statement, left bound of iter_space must be less or equal to right bound");
	}

    return new_iter;
}

expr_list_t *expr_list_add(expr_list_t *new_list, expr_t *l) {
    expr_list_t *list;

    if (!new_list) {
        list = (expr_list_t*)malloc(sizeof(expr_list_t));
        list->expr_list_empty = MAX_EXPR_LIST;
        list->all_expr_num = 0;
    }
    else {
        list = new_list;
    }

    //the supposed number of expressions, assume that there are no NULL expressions
    list->all_expr_num++;

    if (!l) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("ERROR: null expression to expr_list (debugging info) ");
#endif
    }
    else if (list->expr_list_empty) {
        list->expr_list[MAX_EXPR_LIST-list->expr_list_empty] = l;
        list->expr_list_empty--;
    }
    else {
        yyerror("ERROR: too much expressions");
    }
    return list;
}

var_list_t *var_list_add(var_list_t *new_list, var_t *v) {
    var_list_t *list;

    if (!new_list) {
        list = (var_list_t*)malloc(sizeof(var_list_t));
        list->var_list_empty = MAX_VAR_LIST;
        list->all_var_num = 0;
    }
    else {
        list = new_list;
    }

    //the supposed number of expressions, assume that there are no NULL expressions
    list->all_var_num++;

    if (!v) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("ERROR: null variable to var_list (debugging info) ");
#endif
    }
    else if (list->var_list_empty) {
        list->var_list[MAX_VAR_LIST-list->var_list_empty] = v;
        list->var_list_empty--;
    }
    else {
        yyerror("ERROR: too much variables");
    }
    return list;
}

elexpr_list_t *elexpr_list_add(elexpr_list_t *new_list,elexpr_t *el) {
    elexpr_list_t *list;

    if (!new_list) {
        list = (elexpr_list_t*)malloc(sizeof(elexpr_list_t));
        list->elexpr_list_usage = EXPR_NULL_SET;
        list->elexpr_list_empty = MAX_SET_ELEM;
        list->elexpr_list_datatype = TYPE_VOID; //init value, usefull for debugging
        list->all_elexpr_num = 0;
    }
    else {
        list = new_list;
    }

    //the supposed number of elexpressions, assume that there are no NULL elexpressions
    list->all_elexpr_num++;

    if (!el) {
#if BISON_DEBUG_LEVEL >= 2
        yyerror("ERROR: null elexpression to elexpr_list (debugging info) ");
#endif
    }
    else if (list->elexpr_list_empty==MAX_SET_ELEM) {
        //the first element sets the type of the set
        list->elexpr_list_usage = EXPR_SET;
        list->elexpr_list[MAX_SET_ELEM-list->elexpr_list_empty] = el;
        list->elexpr_list_empty--;
        list->elexpr_list_datatype = el->elexpr_datatype;
    }
    else if (list->elexpr_list_empty) {
        //check if datatype is the same with the first element
        if (el->elexpr_datatype!=list->elexpr_list_datatype) {
            //all elexpressions in setexpression must have the same type
            yyerror("ERROR: ignoring elexpression because of type mismatch");
        }
        else {
            //list->elexpr_list_usage = EXPR_SET;
            list->elexpr_list[MAX_SET_ELEM-list->elexpr_list_empty] = el;
            list->elexpr_list_empty--;
        }
    }
    else {
        yyerror("ERROR: too many elexpressions");
    }
    return list;
}

elexpr_t *make_elexpr_range(expr_t *l1, expr_t *l2) {
    elexpr_t *new_elexpr;

    if (!l1 || !l2) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("ERROR: null expression in setexpression (debugging info)");
#endif
        return NULL;
    }

	if (l1->expr_is==EXPR_LOST && l2->expr_is==EXPR_LOST) {
		return NULL; //both expressions had parse errors, ignore elexpr
	}
	//at least one expression exists, check if valid

    if (!TYPE_IS_ELEXPR_VALID(l1->datatype)) {
        sprintf(str_err,"ERROR: ignoring elexpression range because of invalid type '%s'",l1->datatype->data_name);
		yyerror(str_err);
        return NULL;
    }

	if (!TYPE_IS_ELEXPR_VALID(l2->datatype)) {
        sprintf(str_err,"ERROR: ignoring elexpression range because of invalid type '%s'",l2->datatype->data_name);
		yyerror(str_err);
        return NULL;
    }


	if (l1->datatype!=l2->datatype) {
        yyerror("ERROR: ignoring elexpression range because bounds don't have the same type");
        return NULL;
    }

	if (l1->expr_is==EXPR_HARDCODED_CONST && l2->expr_is==EXPR_HARDCODED_CONST) { //optimization
		if (l1->ival>l2->ival) {
			sprintf(str_err,"WARNING: ignore invalid range %d..%d, left bound must be less or equal to right bound",l1->ival,l2->ival);
			yyerror(str_err);
			return NULL;
		}
	}

    new_elexpr = (elexpr_t*)malloc(sizeof(elexpr_t));
	if (l1->expr_is==EXPR_LOST) {
		new_elexpr->left = l2;
		new_elexpr->right = l2;
		new_elexpr->elexpr_datatype = l2->datatype;
	}
	else if (l2->expr_is==EXPR_LOST) {
		new_elexpr->left = l1;
		new_elexpr->right = l1;
		new_elexpr->elexpr_datatype = l1->datatype;
	}
	else {
		new_elexpr->left = l1;
		new_elexpr->right = l2;
		new_elexpr->elexpr_datatype = l1->datatype;
	}

    return new_elexpr;
}

elexpr_t *make_elexpr(expr_t *l) {
    elexpr_t *new_elexpr;

    if (!l) {
#if BISON_DEBUG_LEVEL >= 1
        yyerror("ERROR: null expression in setexpression (debugging info)");
#endif
        return NULL;
    }

	if (l->expr_is==EXPR_LOST) {
		return NULL; //we had parse errors, ignore elexpr
	}

    if (!TYPE_IS_ELEXPR_VALID(l->datatype)) {
        sprintf(str_err,"ERROR: ignoring elexpression because of invalid type '%s'",l->datatype->data_name);
		yyerror(str_err);
        return NULL;
    }

    //this is one element of a set and not a range, so both
    //expressions of struct elexpr_t are the same
    new_elexpr = (elexpr_t*)malloc(sizeof(elexpr_t));
    new_elexpr->left = l;
    new_elexpr->right = l;
    new_elexpr->elexpr_datatype = l->datatype;

    return new_elexpr;
}

expr_t *limit_from_id(char *id) {
    sem_t *sem_1;
    expr_t *new_expr;

    sem_1 = sm_find(id);
    if (sem_1) {
        if (sem_1->id_is == ID_CONST) {
            if (sem_1->var->datatype->is==TYPE_INT) {
				new_expr = expr_from_hardcoded_int(sem_1->var->ival);
			}
			else if (sem_1->var->datatype->is==TYPE_BOOLEAN) {
                new_expr = expr_from_hardcoded_boolean(sem_1->var->ival);
            }
            else if (sem_1->var->datatype->is==TYPE_CHAR) {
                new_expr = expr_from_hardcoded_char(sem_1->var->cval);
            }
            else {
                yyerror("ERROR: limit can't be real type");
				new_expr = expr_from_hardcoded_int(0);
            }
        }
        else if (sem_1->id_is==ID_TYPEDEF && sem_1->comp->is==TYPE_ENUM) {
            if (strcmp(sem_1->comp->data_name,id)==0) {
                sprintf(str_err,"ERROR: limit '%s' is a typename, expected constant integer or enumeration",id);
				yyerror(str_err);
				new_expr = expr_from_hardcoded_int(0);
            }
            else {
                new_expr = expr_from_hardcoded_int(enum_num_of_id(sem_1->comp,id));
				new_expr->datatype = sem_1->comp;
                return new_expr;
            }
        }
        else {
			sprintf(str_err,"ERROR: invalid limit '%s', expected constant integer or enumeration",id);
			yyerror(str_err);
			new_expr = expr_from_hardcoded_int(0);
        }
    }
    else {
        if (!sm_find_lost_symbol(id)) {
            sm_insert_lost_symbol(id);
            sprintf(str_err,"ERROR: undeclared symbol '%s'",id);
            yyerror(str_err);
        }
		new_expr = expr_from_hardcoded_int(0);
    }
    return new_expr;
}

expr_t *limit_from_signed_id(op_t op,char *id) {
    sem_t *sem_2;
    expr_t *new_expr;

    sem_2 = sm_find(id);
    if (sem_2) {
        if (sem_2->id_is == ID_CONST) {
            if (sem_2->var->datatype->is==TYPE_INT) {
				new_expr = expr_from_signed_hardcoded_int(op,sem_2->var->ival);
            }
            else {
                yyerror("ERROR: signed `limit` can be only integer constant");
				new_expr = expr_from_hardcoded_int(0);
            }
        }
        else {
            yyerror("ERROR: signed `limit` can be only integer constant");
			new_expr = expr_from_hardcoded_int(0);
        }
    }
    else {
        if (!sm_find_lost_symbol(id)) {
            sm_insert_lost_symbol(id);
            sprintf(str_err,"ERROR: undeclared symbol '%s'",id);
            yyerror(str_err);
        }
		new_expr = expr_from_hardcoded_int(0);
    }
    return new_expr;
}

data_t *reference_to_typename(char *id) {
  sem_t *sm_1;
  sm_1 = sm_find(id);
  if (sm_1) {
    if (sm_1->id_is == ID_TYPEDEF) {
      return sm_1->comp;
    }
	yyerror("ERROR: id is not a data type");
  }
  else {
    if (!sm_find_lost_symbol(id)) {
		sm_insert_lost_symbol(id);
		sprintf(str_err,"ERROR: undeclared datatype '%s'",id);
		yyerror(str_err);
	}
  }
  return NULL; //we return NULL here because we handle differently the various cases
  //with reference to user-defined datatypes
}
