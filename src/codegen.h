#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

typedef struct {
    FILE *out;             // open output file (.s), owned by the caller
    const char *func_name; // points into AST buffer (not null-terminated)
    int func_name_len;
    int label_count;    // monotonically increasing label counter
    int errors;         // count of fatal errors (non-zero → codegen_exec returns -1)
    int loop_label;     // label number of innermost loop, -1 if not in a loop
    int loop_cont_kind; // continue target: 0=_start (while), 1=_cond (do-while), 2=_post (for)
    int break_label;    // label number for break target (innermost loop or switch), -1 if none
    struct node **switch_cases;     // current switch case node pointers (temporary during switch codegen)
    int *switch_case_labels;        // parallel array of label numbers for each case
    int switch_ncases;              // number of cases in current switch
    int switch_default_lbl;         // label number for default, -1 if none
    int str_count;                  // counter for .LC string literal labels
} codegen_t;

void codegen_init(codegen_t *cg, FILE *out);
void codegen_finish(codegen_t *cg);
int codegen_exec(codegen_t *cg, node_t *root);

#endif /* CODEGEN_H */
