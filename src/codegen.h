#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

typedef struct {
    FILE *out;             // open output file (.s), owned by the caller
    const char *func_name; // points into AST buffer (not null-terminated)
    int func_name_len;
    int label_count;      // monotonically increasing label counter
    int errors;           // count of fatal errors (non-zero → codegen_exec returns -1)
    int loop_label;       // label number of innermost loop, -1 if not in a loop
    int loop_is_do_while; // 1 when innermost loop is do-while (continue → _cond), 0 otherwise
} codegen_t;

void codegen_init(codegen_t *cg, FILE *out);
void codegen_finish(codegen_t *cg);
int codegen_exec(codegen_t *cg, node_t *root);

#endif /* CODEGEN_H */
