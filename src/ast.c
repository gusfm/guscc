#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include "sym.h"

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
    if (node == NULL)
        return;
    switch (node->kind) {
        case ND_TRANSLATION_UNIT:
            for (int i = 0; i < node->translation_unit.nfuncs; i++)
                node_destroy(node->translation_unit.funcs[i]);
            break;
        case ND_FUNC:
            node_destroy(node->func.decl_spec);
            node_destroy(node->func.declarator);
            node_destroy(node->func.comp_stmt);
            sym_destroy_list(node->func.params_sym_list);
            break;
        case ND_DECL_SPEC:
            node_destroy(node->decl_spec.type_spec);
            break;
        case ND_PARAM_DECL:
            node_destroy(node->param_decl.decl_spec);
            node_destroy(node->param_decl.declarator);
            break;
        case ND_PARAM_LIST:
            for (int i = 0; i < node->param_list.nparams; i++)
                node_destroy(node->param_list.params[i]);
            break;
        case ND_DIRECT_DECL:
            node_destroy(node->direct_decl.param_list);
            break;
        case ND_COMP_STMT:
            for (int i = 0; i < node->comp_stmt.nstmts; i++)
                node_destroy(node->comp_stmt.stmts[i]);
            break;
        case ND_IF_STMT:
            node_destroy(node->if_stmt.cond);
            node_destroy(node->if_stmt.then);
            node_destroy(node->if_stmt.else_);
            break;
        case ND_WHILE_STMT:
            node_destroy(node->while_stmt.cond);
            node_destroy(node->while_stmt.body);
            break;
        case ND_BREAK_STMT:
        case ND_CONTINUE_STMT:
            break;
        case ND_RETURN_STMT:
            node_destroy(node->return_stmt.expr);
            break;
        case ND_EXPR_STMT:
            node_destroy(node->expr_stmt.expr);
            break;
        case ND_BINOP:
            node_destroy(node->binop.left);
            node_destroy(node->binop.right);
            break;
        case ND_UNOP:
            node_destroy(node->unop.operand);
            break;
        case ND_POSTOP:
            node_destroy(node->postop.operand);
            break;
        case ND_SUBSCRIPT:
            node_destroy(node->subscript.array);
            node_destroy(node->subscript.index);
            break;
        case ND_CALL:
            node_destroy(node->call.func);
            for (int i = 0; i < node->call.nargs; i++)
                node_destroy(node->call.args[i]);
            break;
        case ND_MEMBER:
            node_destroy(node->member.object);
            break;
        case ND_CAST:
            node_destroy(node->cast.type_node);
            node_destroy(node->cast.expr);
            break;
        case ND_SIZEOF_EXPR:
            node_destroy(node->sizeof_expr.expr);
            break;
        case ND_SIZEOF_TYPE:
            node_destroy(node->sizeof_type.type_node);
            break;
        case ND_TERNARY:
            node_destroy(node->ternary.cond);
            node_destroy(node->ternary.then_expr);
            node_destroy(node->ternary.else_expr);
            break;
        case ND_ASSIGN:
            node_destroy(node->assign.lhs);
            node_destroy(node->assign.rhs);
            break;
        case ND_COMMA:
            node_destroy(node->comma.left);
            node_destroy(node->comma.right);
            break;
        case ND_LOCAL_DECL:
            node_destroy(node->local_decl.decl_spec);
            node_destroy(node->local_decl.declarator);
            node_destroy(node->local_decl.init);
            free(node->local_decl.sym);
            break;
        case ND_TYPE_SPEC:
        case ND_NUM:
        case ND_IDENT:
        case ND_STR:
            break;
    }
    free(node);
}

static void print_indent(int indent)
{
    for (int i = 0; i < indent; i++)
        printf("  ");
}

static const char *op_to_str(int op, char *buf)
{
    switch (op) {
        case TOKEN_INC_OP:
            return "++";
        case TOKEN_DEC_OP:
            return "--";
        case TOKEN_PTR_OP:
            return "->";
        case TOKEN_LEFT_OP:
            return "<<";
        case TOKEN_RIGHT_OP:
            return ">>";
        case TOKEN_LE_OP:
            return "<=";
        case TOKEN_GE_OP:
            return ">=";
        case TOKEN_EQ_OP:
            return "==";
        case TOKEN_NE_OP:
            return "!=";
        case TOKEN_AND_OP:
            return "&&";
        case TOKEN_OR_OP:
            return "||";
        case TOKEN_MUL_ASSIGN:
            return "*=";
        case TOKEN_DIV_ASSIGN:
            return "/=";
        case TOKEN_MOD_ASSIGN:
            return "%=";
        case TOKEN_ADD_ASSIGN:
            return "+=";
        case TOKEN_SUB_ASSIGN:
            return "-=";
        case TOKEN_LEFT_ASSIGN:
            return "<<=";
        case TOKEN_RIGHT_ASSIGN:
            return ">>=";
        case TOKEN_AND_ASSIGN:
            return "&=";
        case TOKEN_XOR_ASSIGN:
            return "^=";
        case TOKEN_OR_ASSIGN:
            return "|=";
        default:
            buf[0] = (char)op;
            buf[1] = '\0';
            return buf;
    }
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
        case ND_TRANSLATION_UNIT:
            printf("Translation unit: %d function(s)\n", n->translation_unit.nfuncs);
            for (int i = 0; i < n->translation_unit.nfuncs; i++)
                ast_print(n->translation_unit.funcs[i], indent + 1);
            break;
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
            printf("Type specifier:%d:%d: %s\n", n->line, n->col, type_spec_to_str(n->type_spec));
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
            printf("%.*s pointer_level=%d\n", n->direct_decl.ident.len, n->direct_decl.ident.str,
                   n->direct_decl.pointer_level);
            if (n->direct_decl.param_list) {
                ast_print(n->direct_decl.param_list, indent + 1);
            }
            break;
        case ND_COMP_STMT:
            printf("Compound statement:%d:%d:\n", n->line, n->col);
            for (int i = 0; i < n->comp_stmt.nstmts; ++i)
                ast_print(n->comp_stmt.stmts[i], indent + 1);
            break;
        case ND_IF_STMT:
            printf("IfStatement:%d:%d:\n", n->line, n->col);
            ast_print(n->if_stmt.cond, indent + 1);
            ast_print(n->if_stmt.then, indent + 1);
            if (n->if_stmt.else_)
                ast_print(n->if_stmt.else_, indent + 1);
            break;
        case ND_WHILE_STMT:
            printf("WhileStatement:%d:%d:\n", n->line, n->col);
            ast_print(n->while_stmt.cond, indent + 1);
            ast_print(n->while_stmt.body, indent + 1);
            break;
        case ND_BREAK_STMT:
            printf("BreakStatement:%d:%d:\n", n->line, n->col);
            break;
        case ND_CONTINUE_STMT:
            printf("ContinueStatement:%d:%d:\n", n->line, n->col);
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
            printf("Number:%d:%d: %.*s\n", n->line, n->col, n->num.val.len, n->num.val.str);
            break;
        case ND_IDENT:
            if (n->ident.sym)
                printf("Identifier:%d:%d: %.*s (offset=%d)\n", n->line, n->col, n->ident.name.len,
                       n->ident.name.str, n->ident.sym->offset);
            else
                printf("Identifier:%d:%d: %.*s (unresolved)\n", n->line, n->col, n->ident.name.len,
                       n->ident.name.str);
            break;
        case ND_STR:
            printf("String:%d:%d: %.*s\n", n->line, n->col, n->str.val.len, n->str.val.str);
            break;
        case ND_BINOP: {
            char buf[4];
            printf("BinOp:%d:%d: %s\n", n->line, n->col, op_to_str(n->binop.op, buf));
            ast_print(n->binop.left, indent + 1);
            ast_print(n->binop.right, indent + 1);
            break;
        }
        case ND_UNOP: {
            char buf[4];
            printf("UnaryOp:%d:%d: %s\n", n->line, n->col, op_to_str(n->unop.op, buf));
            ast_print(n->unop.operand, indent + 1);
            break;
        }
        case ND_POSTOP: {
            char buf[4];
            printf("PostfixOp:%d:%d: %s\n", n->line, n->col, op_to_str(n->postop.op, buf));
            ast_print(n->postop.operand, indent + 1);
            break;
        }
        case ND_SUBSCRIPT:
            printf("Subscript:%d:%d:\n", n->line, n->col);
            ast_print(n->subscript.array, indent + 1);
            ast_print(n->subscript.index, indent + 1);
            break;
        case ND_CALL:
            printf("Call:%d:%d:\n", n->line, n->col);
            ast_print(n->call.func, indent + 1);
            for (int i = 0; i < n->call.nargs; i++)
                ast_print(n->call.args[i], indent + 1);
            break;
        case ND_MEMBER:
            printf("Member:%d:%d: %s%.*s\n", n->line, n->col, n->member.is_ptr ? "->" : ".",
                   n->member.field.len, n->member.field.str);
            ast_print(n->member.object, indent + 1);
            break;
        case ND_CAST:
            printf("Cast:%d:%d:\n", n->line, n->col);
            ast_print(n->cast.type_node, indent + 1);
            ast_print(n->cast.expr, indent + 1);
            break;
        case ND_SIZEOF_EXPR:
            printf("SizeofExpr:%d:%d:\n", n->line, n->col);
            ast_print(n->sizeof_expr.expr, indent + 1);
            break;
        case ND_SIZEOF_TYPE:
            printf("SizeofType:%d:%d:\n", n->line, n->col);
            ast_print(n->sizeof_type.type_node, indent + 1);
            break;
        case ND_TERNARY:
            printf("Ternary:%d:%d:\n", n->line, n->col);
            ast_print(n->ternary.cond, indent + 1);
            ast_print(n->ternary.then_expr, indent + 1);
            ast_print(n->ternary.else_expr, indent + 1);
            break;
        case ND_ASSIGN: {
            char buf[4];
            printf("Assign:%d:%d: %s\n", n->line, n->col, op_to_str(n->assign.op, buf));
            ast_print(n->assign.lhs, indent + 1);
            ast_print(n->assign.rhs, indent + 1);
            break;
        }
        case ND_COMMA:
            printf("Comma:%d:%d:\n", n->line, n->col);
            ast_print(n->comma.left, indent + 1);
            ast_print(n->comma.right, indent + 1);
            break;
        case ND_LOCAL_DECL:
            printf("LocalDecl:%d:%d: %.*s (offset=%d)\n", n->line, n->col,
                   n->local_decl.declarator->direct_decl.ident.len,
                   n->local_decl.declarator->direct_decl.ident.str,
                   n->local_decl.sym ? n->local_decl.sym->offset : 0);
            ast_print(n->local_decl.decl_spec, indent + 1);
            if (n->local_decl.init)
                ast_print(n->local_decl.init, indent + 1);
            break;
    }
}
