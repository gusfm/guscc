#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
static void cg_node(codegen_t *cg, node_t *n);
static void cg_expr(codegen_t *cg, node_t *n);

void codegen_init(codegen_t *cg, FILE *out)
{
    cg->out = out;
    cg->func_name = NULL;
    cg->func_name_len = 0;
}

void codegen_finish(codegen_t *cg)
{
    cg->out = NULL;
}

static void cg_num(codegen_t *cg, node_t *n)
{
    char tmp[64];
    int len = n->num.val.len < 63 ? n->num.val.len : 63;
    memcpy(tmp, n->num.val.str, len);
    tmp[len] = '\0';
    long val = strtol(tmp, NULL, 10);
    fprintf(cg->out, "\tmovl\t$%ld, %%eax\n", val);
}

static void cg_expr(codegen_t *cg, node_t *n)
{
    if (n == NULL)
        return;
    switch (n->kind) {
        case ND_NUM:
            cg_num(cg, n);
            break;
        default:
            fprintf(stderr, "codegen: unsupported expression kind %d\n",
                    n->kind);
            break;
    }
}

static void cg_return_stmt(codegen_t *cg, node_t *n)
{
    if (n->return_stmt.expr != NULL)
        cg_expr(cg, n->return_stmt.expr);
    fprintf(cg->out, "\tjmp\t.L%.*s_end\n", cg->func_name_len, cg->func_name);
}

static void cg_expr_stmt(codegen_t *cg, node_t *n)
{
    if (n->expr_stmt.expr != NULL)
        cg_expr(cg, n->expr_stmt.expr);
}

static void cg_comp_stmt(codegen_t *cg, node_t *n)
{
    for (int i = 0; i < n->comp_stmt.nstmts; ++i)
        cg_node(cg, n->comp_stmt.stmts[i]);
}

static void cg_func(codegen_t *cg, node_t *n)
{
    node_str_t name = n->func.declarator->direct_decl.ident;

    cg->func_name = name.str;
    cg->func_name_len = name.len;

    fprintf(cg->out, "\t.text\n");
    fprintf(cg->out, "\t.globl\t%.*s\n", name.len, name.str);
    fprintf(cg->out, "\t.type\t%.*s, @function\n", name.len, name.str);
    fprintf(cg->out, "%.*s:\n", name.len, name.str);
    fprintf(cg->out, "\tpushq\t%%rbp\n");
    fprintf(cg->out, "\tmovq\t%%rsp, %%rbp\n");

    cg_node(cg, n->func.comp_stmt);

    fprintf(cg->out, ".L%.*s_end:\n", name.len, name.str);
    fprintf(cg->out, "\tpopq\t%%rbp\n");
    fprintf(cg->out, "\tret\n");
}

static void cg_node(codegen_t *cg, node_t *n)
{
    if (n == NULL)
        return;
    switch (n->kind) {
        case ND_FUNC:
            cg_func(cg, n);
            break;
        case ND_COMP_STMT:
            cg_comp_stmt(cg, n);
            break;
        case ND_RETURN_STMT:
            cg_return_stmt(cg, n);
            break;
        case ND_EXPR_STMT:
            cg_expr_stmt(cg, n);
            break;
        case ND_NUM:
            cg_num(cg, n);
            break;
        case ND_DECL_SPEC:
        case ND_TYPE_SPEC:
        case ND_PARAM_DECL:
        case ND_PARAM_LIST:
        case ND_DIRECT_DECL:
            break;
    }
}

int codegen_exec(codegen_t *cg, node_t *root)
{
    if (root == NULL)
        return -1;
    cg_node(cg, root);
    return 0;
}
