#include <stdio.h>

#include "generator.h"

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
        if (!statement_root_module[i] ) {
            break;
        }
        ir_tree = generate_module(statement_root_module[i]);
        ir_tree = append_return_node(ir_tree);
        ir_root_tree[i] = link_ir_to_ir(ir_tree,ir_root_tree[i]);
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
    ir_node_t *ir_new;
    ir_node_t *ir_tmp1;
    ir_node_t *ir_tmp2;

    if (!s) {
        return NULL;
    }

    if (s->type==ST_BadStatement) {
        return new_lost_ir_node("__BAD_STATEMENT__");
    }

    switch (s->type) {
    case ST_If:
        ir_tmp1 = generate_ir_from_statement(s->_if._true);
        ir_tmp2 = generate_ir_from_statement(s->_if._false);
        ir_new = new_ir_if(s->_if.condition,
                           ir_tmp1,
                           ir_tmp2);
        break;
    case ST_While:
        ir_tmp1 = generate_ir_from_statement(s->_while.loop);
        ir_new = new_ir_while(s->_while.condition,
                              ir_tmp1);
        break;
    case ST_For:
        ir_tmp1 = generate_ir_from_statement(s->_for.loop);
        ir_new = new_ir_for(s->_for.var,
                            s->_for.iter,
                            ir_tmp1);
        break;
    case ST_Call:
        ir_new = new_ir_procedure_call(s->_call.subprogram,
                                       s->_call.expr_params);
        break;
    case ST_Assignment:
        ir_new = new_ir_assign(s->_assignment.var,s->_assignment.expr);
        break;
    case ST_With:
        ir_tmp1 = generate_ir_from_statement(s->_with.statement);
        ir_new = new_ir_with(ir_tmp1);
        break;
    case ST_Read:
        ir_new = new_ir_read(s->_read.var_list);
        break;
    case ST_Write:
        ir_new = new_ir_write(s->_write.expr_list);
        break;
    case ST_BadStatement:
        ir_new = new_lost_ir_node("__BAD_STATEMENT__");
        break;
    }
    return ir_new;
}
