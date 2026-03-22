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
    cg->label_count = 0;
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

static void cg_binop(codegen_t *cg, node_t *n)
{
    int op = n->binop.op;

    // Short-circuit &&
    if (op == TOKEN_AND_OP) {
        int lbl = cg->label_count++;
        cg_expr(cg, n->binop.left);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tje\t.L%d\n", lbl);
        cg_expr(cg, n->binop.right);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tsetne\t%%al\n");
        fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
        fprintf(cg->out, "\tjmp\t.L%d_end\n", lbl);
        fprintf(cg->out, ".L%d:\n", lbl);
        fprintf(cg->out, "\txorl\t%%eax, %%eax\n");
        fprintf(cg->out, ".L%d_end:\n", lbl);
        return;
    }

    // Short-circuit ||
    if (op == TOKEN_OR_OP) {
        int lbl = cg->label_count++;
        cg_expr(cg, n->binop.left);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tjne\t.L%d\n", lbl);
        cg_expr(cg, n->binop.right);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tsetne\t%%al\n");
        fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
        fprintf(cg->out, "\tjmp\t.L%d_end\n", lbl);
        fprintf(cg->out, ".L%d:\n", lbl);
        fprintf(cg->out, "\tmovl\t$1, %%eax\n");
        fprintf(cg->out, ".L%d_end:\n", lbl);
        return;
    }

    // General case: left → %rax, push; right → %rax; pop left into %rcx
    cg_expr(cg, n->binop.left);
    fprintf(cg->out, "\tpushq\t%%rax\n");
    cg_expr(cg, n->binop.right);
    fprintf(cg->out, "\tpopq\t%%rcx\n");
    // left in %ecx, right in %eax

    switch (op) {
        case '+':
            fprintf(cg->out, "\taddl\t%%ecx, %%eax\n");
            break;
        case '-':
            // left - right = %ecx - %eax
            fprintf(cg->out, "\tsubl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            break;
        case '*':
            fprintf(cg->out, "\timull\t%%ecx, %%eax\n");
            break;
        case '/':
            // %ecx / %eax — save divisor, move dividend to eax, cdq, idivl
            fprintf(cg->out, "\tmovl\t%%eax, %%r8d\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            fprintf(cg->out, "\tcdq\n");
            fprintf(cg->out, "\tidivl\t%%r8d\n");
            break;
        case '%':
            fprintf(cg->out, "\tmovl\t%%eax, %%r8d\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            fprintf(cg->out, "\tcdq\n");
            fprintf(cg->out, "\tidivl\t%%r8d\n");
            fprintf(cg->out, "\tmovl\t%%edx, %%eax\n");
            break;
        case TOKEN_EQ_OP:
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsete\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case TOKEN_NE_OP:
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetne\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case '<':
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetl\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case '>':
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetg\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case TOKEN_LE_OP:
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetle\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case TOKEN_GE_OP:
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetge\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case '&':
            fprintf(cg->out, "\tandl\t%%ecx, %%eax\n");
            break;
        case '|':
            fprintf(cg->out, "\torl\t%%ecx, %%eax\n");
            break;
        case '^':
            fprintf(cg->out, "\txorl\t%%ecx, %%eax\n");
            break;
        case TOKEN_LEFT_OP:
            // left (ecx) << right (eax) — count must be in %cl
            fprintf(cg->out, "\tmovl\t%%eax, %%r8d\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            fprintf(cg->out, "\tmovl\t%%r8d, %%ecx\n");
            fprintf(cg->out, "\tsall\t%%cl, %%eax\n");
            break;
        case TOKEN_RIGHT_OP:
            fprintf(cg->out, "\tmovl\t%%eax, %%r8d\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            fprintf(cg->out, "\tmovl\t%%r8d, %%ecx\n");
            fprintf(cg->out, "\tsarl\t%%cl, %%eax\n");
            break;
        default:
            fprintf(stderr, "codegen: unsupported binary op %d\n", op);
            break;
    }
}

static void cg_unop(codegen_t *cg, node_t *n)
{
    int op = n->unop.op;
    cg_expr(cg, n->unop.operand);
    switch (op) {
        case '-':
            fprintf(cg->out, "\tnegl\t%%eax\n");
            break;
        case '+':
            // unary + is a no-op
            break;
        case '!':
            fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
            fprintf(cg->out, "\tsete\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case '~':
            fprintf(cg->out, "\tnotl\t%%eax\n");
            break;
        default:
            fprintf(stderr, "codegen: unsupported unary op %d\n", op);
            break;
    }
}

static void cg_ternary(codegen_t *cg, node_t *n)
{
    int lbl = cg->label_count++;
    cg_expr(cg, n->ternary.cond);
    fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
    fprintf(cg->out, "\tje\t.L%d\n", lbl);
    cg_expr(cg, n->ternary.then_expr);
    fprintf(cg->out, "\tjmp\t.L%d_end\n", lbl);
    fprintf(cg->out, ".L%d:\n", lbl);
    cg_expr(cg, n->ternary.else_expr);
    fprintf(cg->out, ".L%d_end:\n", lbl);
}

static void cg_cast(codegen_t *cg, node_t *n)
{
    cg_expr(cg, n->cast.expr);
    node_t *ds = n->cast.type_node;
    if (ds && ds->kind == ND_DECL_SPEC && ds->decl_spec.pointer_level == 0 &&
        ds->decl_spec.type_spec &&
        ds->decl_spec.type_spec->type_spec == ND_TYPE_CHAR) {
        fprintf(cg->out, "\tmovsbl\t%%al, %%eax\n");
    }
    // int and pointer casts: no truncation needed (eax is already 32-bit)
}

static void cg_sizeof_type(codegen_t *cg, node_t *n)
{
    int size = 4;
    node_t *ds = n->sizeof_type.type_node;
    if (ds && ds->kind == ND_DECL_SPEC) {
        if (ds->decl_spec.pointer_level > 0) {
            size = 8;
        } else if (ds->decl_spec.type_spec) {
            switch (ds->decl_spec.type_spec->type_spec) {
                case ND_TYPE_CHAR:
                    size = 1;
                    break;
                case ND_TYPE_INT:
                    size = 4;
                    break;
                case ND_TYPE_VOID:
                    size = 1;
                    break;
            }
        }
    }
    fprintf(cg->out, "\tmovl\t$%d, %%eax\n", size);
}

static void cg_comma(codegen_t *cg, node_t *n)
{
    cg_expr(cg, n->comma.left);
    cg_expr(cg, n->comma.right);
}

static void cg_expr(codegen_t *cg, node_t *n)
{
    if (n == NULL)
        return;
    switch (n->kind) {
        case ND_NUM:
            cg_num(cg, n);
            break;
        case ND_BINOP:
            cg_binop(cg, n);
            break;
        case ND_UNOP:
            cg_unop(cg, n);
            break;
        case ND_TERNARY:
            cg_ternary(cg, n);
            break;
        case ND_CAST:
            cg_cast(cg, n);
            break;
        case ND_SIZEOF_TYPE:
            cg_sizeof_type(cg, n);
            break;
        case ND_COMMA:
            cg_comma(cg, n);
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
        case ND_DECL_SPEC:
        case ND_TYPE_SPEC:
        case ND_PARAM_DECL:
        case ND_PARAM_LIST:
        case ND_DIRECT_DECL:
            break;
        default:
            // expression nodes that appear as statements
            cg_expr(cg, n);
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
