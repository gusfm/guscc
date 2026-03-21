#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

typedef struct {
    FILE *out;             // open output file (.s), owned by the caller
    const char *func_name; // points into AST buffer (not null-terminated)
    int func_name_len;
} codegen_t;

void codegen_init(codegen_t *cg, FILE *out);
void codegen_finish(codegen_t *cg);
int codegen_exec(codegen_t *cg, node_t *root);

#endif /* CODEGEN_H */
