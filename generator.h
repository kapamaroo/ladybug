#ifndef _GENERATOR_H
#define _GENERATOR_H

#include "statements.h"
#include "ir.h"

void generate_all_modules();
ir_node_t *generate_module(statement_t *module);
ir_node_t *generate_ir_from_statement(statement_t *s);

#endif
