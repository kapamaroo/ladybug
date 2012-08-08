#include <stdio.h>

#include "statements.h"
#include "ir.h"

ir_node_t *generate_ir_from_statement(statement_t *s) {
    ir_node_t *ir_new;
    ir_node_t *ir_tmp1;
    ir_node_t *ir_tmp2;
    ir_node_t *ir_tmp;
    statement_t *s_tmp;
    var_t *var;

    if (!s) {
        return NULL;
    }

    if (s->type==ST_BadStatement) {
        //ignore statement
        return NULL;
    }

    switch (s->type) {
    case ST_If:

        ir_tmp1 = NULL;
        s_tmp = s->_if._true;
        while (s_tmp) {
            ir_tmp = generate_ir_from_statement(s_tmp);
            ir_tmp1 = link_ir_to_ir(ir_tmp,ir_tmp1);
            s_tmp = s_tmp->next;
        }

        ir_tmp2 = NULL;
        s_tmp = s->_if._false;
        while (s_tmp) {
            ir_tmp = generate_ir_from_statement(s_tmp);
            ir_tmp2 = link_ir_to_ir(ir_tmp,ir_tmp2);
            s_tmp = s_tmp->next;
        }

        ir_new = new_ir_if(s->_if.condition,
                           ir_tmp1,
                           ir_tmp2);
        break;
    case ST_While:
        ir_new = generate_ir_from_statement(s->_for.prologue);

        ir_tmp = generate_ir_from_statement(s->_while.loop);

        ir_tmp1 = new_ir_while(s->_while.condition,
                              ir_tmp);

        ir_tmp2 = generate_ir_from_statement(s->_for.epilogue);

        ir_new = link_ir_to_ir(ir_tmp1,ir_new);
        ir_new = link_ir_to_ir(ir_tmp2,ir_new);

        break;
    case ST_For:
        ir_new = generate_ir_from_statement(s->_for.prologue);
        ir_tmp = generate_ir_from_statement(s->_for.loop);

        ir_tmp1 = new_ir_for(s->_for.var,
                            s->_for.iter,
                            ir_tmp);

        ir_tmp2 = generate_ir_from_statement(s->_for.epilogue);

        ir_new = link_ir_to_ir(ir_tmp1,ir_new);
        ir_new = link_ir_to_ir(ir_tmp2,ir_new);

        break;
    case ST_Call:
        ir_new = new_ir_procedure_call(s->_call.subprogram,
                                       s->_call.expr_params);
        break;
    case ST_Assignment:
        ir_new = new_ir_assign(s->_assignment.var,
                               s->_assignment.expr);
        break;
    case ST_With:
        ir_tmp1 = generate_ir_from_statement(s->_with.body);
        ir_new = new_ir_with(ir_tmp1);
        break;
    case ST_Read:
        ir_new = new_ir_read(s->_read.var_list);
        break;
    case ST_Write:
        ir_new = new_ir_write(s->_write.expr_list);
        break;
    case ST_Comp:
        ir_new = NULL;
        s_tmp = s->_comp.first_stmt;
        while (s_tmp) {
            ir_tmp = generate_ir_from_statement(s_tmp);
            ir_new = link_ir_to_ir(ir_tmp,ir_new);
            s_tmp = s_tmp->next;
        }
        break;
    case ST_BadStatement:
        //ignore statement
        ir_new = NULL;
        break;
    }
    return ir_new;
}

ir_node_t *append_return_node(ir_node_t *ir_tree) {
    ir_node_t *ir_return;

    ir_return = new_ir_node_t(NODE_RETURN_SUBPROGRAM);
    ir_tree = link_ir_to_ir(ir_return,ir_tree);

    return ir_tree;
}

ir_node_t *append_exit_node(ir_node_t *ir_tree) {
    ir_node_t *ir_exit;

    ir_exit = new_ir_node_t(NODE_SYSCALL);
    ir_exit->syscall_num = SVC_EXIT;
    ir_tree = link_ir_to_ir(ir_exit,ir_tree);

    return ir_tree;
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

void generate_all_modules() {
    int i;
    ir_node_t *ir_tree;

    //main program (index 0), append exit code
    ir_tree = generate_module(statement_root_module[0]);
    ir_tree = append_exit_node(ir_tree);
    ir_root_tree[0] = link_ir_to_ir(ir_tree,ir_root_tree[0]);

    //start from the first subprogram
    for(i=1;i<MAX_NUM_OF_MODULES;i++) {
        if (statement_root_module[i]) {
            ir_tree = generate_module(statement_root_module[i]);
            ir_tree = append_return_node(ir_tree);
            ir_root_tree[i] = link_ir_to_ir(ir_tree,ir_root_tree[i]);
        }
    }
}
