#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

node_t *node_create(node_kind_t kind, int line, int col)
{
    node_t *n = calloc(1, sizeof(node_t));
    n->kind = kind;
    n->line = line;
    n->col = col;
    return n;
}

void node_destroy(node_t *node)
{
    free(node);
}

static void print_indent(int indent)
{
    for (int i = 0; i < indent; i++)
        printf("  ");
}

static const char *type_spec_to_str(int type_spec)
{
    switch (type_spec) {
        case ND_TYPE_VOID:
            return "void";
        case ND_TYPE_CHAR:
            return "char";
        case ND_TYPE_INT:
            return "int";
    }
    return "invalid type";
}

void ast_print(node_t *n, int indent)
{
    print_indent(indent);
    if (n == NULL) {
        printf("NULL\n");
        return;
    }
    switch (n->kind) {
        case ND_FUNC:
            printf("Function definition:%d:%d:\n", n->line, n->col);
            ast_print(n->func.decl_spec, indent + 1);
            ast_print(n->func.declarator, indent + 1);
            ast_print(n->func.comp_stmt, indent + 1);
            break;
        case ND_DECL_SPEC:
            printf("Declaration specifiers:%d:%d:\n", n->line, n->col);
            ast_print(n->decl_spec.type_spec, indent + 1);
            break;
        case ND_TYPE_SPEC:
            printf("Type specifier:%d:%d: %s\n", n->line, n->col,
                   type_spec_to_str(n->type_spec));
            break;
        case ND_PARAM_DECL:
            printf("Parameter declaration:%d:%d:\n", n->line, n->col);
            ast_print(n->param_decl.decl_spec, indent + 1);
            ast_print(n->param_decl.declarator, indent + 1);
            break;
        case ND_PARAM_LIST:
            printf("Parameter list:%d:%d:\n", n->line, n->col);
            for (int i = 0; i < n->param_list.nparams; ++i) {
                ast_print(n->param_list.params[i], indent + 1);
            }
            break;
        case ND_DIRECT_DECL:
            printf("Direct declarator:%d:%d: ", n->line, n->col);
            printf("%.*s pointer_level=%d\n", n->direct_decl.ident.len,
                   n->direct_decl.ident.str, n->direct_decl.pointer_level);
            if (n->direct_decl.param_list) {
                ast_print(n->direct_decl.param_list, indent + 1);
            }
            break;
        case ND_COMP_STMT:
            printf("Compound statement:%d:%d:\n", n->line, n->col);
            for (int i = 0; i < n->comp_stmt.nstmts; ++i)
                ast_print(n->comp_stmt.stmts[i], indent + 1);
            break;
        case ND_RETURN_STMT:
            printf("Return statement:%d:%d:\n", n->line, n->col);
            if (n->return_stmt.expr)
                ast_print(n->return_stmt.expr, indent + 1);
            break;
        case ND_EXPR_STMT:
            printf("Expression statement:%d:%d:\n", n->line, n->col);
            if (n->expr_stmt.expr)
                ast_print(n->expr_stmt.expr, indent + 1);
            break;
        case ND_NUM:
            printf("Number:%d:%d: %.*s\n", n->line, n->col,
                   n->num.val.len, n->num.val.str);
            break;
    }
}
