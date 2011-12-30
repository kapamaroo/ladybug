#include <stdio.h>

#include "generator.h"
#include "expr_toolbox.h" //for expr_from_STRING //FIXME

ir_node_t *append_return_node(ir_node_t *ir_tree) {
    ir_node_t *ir_return;

    ir_return = new_ir_node_t(NODE_RETURN_SUBPROGRAM);
    ir_tree = link_ir_to_ir(ir_return,ir_tree);

    return ir_tree;
}

void generate_all_modules() {
    int i;
    ir_node_t *ir_tree;

    for(i=0;i<MAX_NUM_OF_MODULES;i++) {
        ir_tree = generate_module(statement_root_module[i]);
        ir_root_tree[i] = link_ir_to_ir(ir_tree,ir_root_tree[i]);
        ir_root_tree[i] = append_return_node(ir_root_tree[i]);
    }
}

ir_node_t *generate_module(statement_t *module) {
    statement_t *current;
    ir_node_t *new_ir;
    ir_node_t *final_ir;

    current = module;
    final_ir = NULL;

    while (current) {
        new_ir = generate_ir_from_statement(current);
        final_ir = link_ir_to_ir(new_ir,final_ir);
        current = current->next;
    }

    return final_ir;
}

ir_node_t *generate_ir_from_statement(statement_t *s) {
    ir_node_t *new_ir;
    expr_t *tmp_expr;

    switch (s->type) {
    case ST_If:
        new_ir = new_ir_if(s->_if.condition,
                           s->_if._true,
                           s->_if._false);
        break;
    case ST_While:
        new_ir = new_ir_while(s->_while.condition,
                              s->_while.loop);
        break;
    case ST_For:
        new_ir = new_ir_for(s->_for.var,
                            s->_for.iter,
                            s->_for.loop);
        break;
    case ST_Call:
        new_ir = new_ir_procedure_call(s->_call.subprogram,
                                       s->_call.expr_params);
        break;
    case ST_Assignment:
        if (s->_assignment.type==AT_String) {
            tmp_expr = expr_from_STRING(s->_assignment.string);
            new_ir = new_ir_assign(s->_assignment.var,
                                   tmp_expr);
        } else {
            new_ir = new_ir_assign(s->_assignment.var,
                                   s->_assignment.expr);
        }
        break;
    case ST_With:
        new_ir = new_ir_with(s->_with.statement);
        break;
    case ST_Read:
        new_ir = new_ir_read(s->_read.var_list);
        break;
    case ST_Write:
        new_ir = new_ir_write(s->_write.expr_list);
        break;
    case ST_BadStatement:
        new_ir = new_lost_ir_node("__BAD_STATEMENT__");
        break;
    }
    return new_ir;
}
