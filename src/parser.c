#include "parser.h"

#include <stdbool.h>
#include <stdio.h>

#include "ast.h"

// Foward declarations
node_t *parser_declarator(parser_t *p);
bool parser_compound_statement(parser_t *p);

void parser_init(parser_t *p, char *buf, size_t size)
{
    lex_init(&p->l, buf, buf + size);
    p->next_token = NULL;
}

void parser_finish(parser_t *p)
{
    if (p->next_token) {
        token_destroy(p->next_token);
        p->next_token = NULL;
    }
}

token_t *parser_peek(parser_t *p)
{
    if (p->next_token == NULL) {
        p->next_token = lex_next(&p->l);
    }
    return p->next_token;
}

token_t *parser_next(parser_t *p)
{
    if (p->next_token != NULL) {
        token_t *t = p->next_token;
        p->next_token = NULL;
        return t;
    }
    return lex_next(&p->l);
}

bool parser_accept(parser_t *p, token_type_t type)
{
    token_t *t = parser_peek(p);
    if (t->type == type) {
        parser_next(p); // consume it
        return true;
    }
    return false;
}

token_t *parser_expect_token(parser_t *p, token_type_t type)
{
    token_t *t = parser_next(p);
    if (t->type == type) {
        return t;
    } else {
        char str[100];
        fprintf(stderr, "Expected %s but received '%.*s'\n",
                token_type_to_str(type, str, 100), t->len, t->sval);
        token_destroy(t);
        return NULL;
    }
}

bool parser_expect(parser_t *p, token_type_t type)
{
    token_t *t = parser_expect_token(p, type);
    if (t != NULL) {
        token_destroy(t);
        return true;
    }
    return false;
}

node_t *parser_type_specifier(parser_t *p)
{
    token_t *tok = parser_next(p);
    node_t *node = node_create(ND_TYPE_SPEC, tok->line, tok->col);
    if (tok->type == TOKEN_KW_VOID) {
        node->type_spec = ND_TYPE_VOID;
    } else if (tok->type == TOKEN_KW_CHAR) {
        node->type_spec = ND_TYPE_CHAR;
    } else if (tok->type == TOKEN_KW_INT) {
        node->type_spec = ND_TYPE_INT;
    } else {
        token_print_error(tok, "type");
        node_destroy(node);
        node = NULL;
    }
    token_destroy(tok);
    return node;
}

node_t *parser_declaration_specifiers(parser_t *p)
{
    node_t *type_spec = parser_type_specifier(p);
    if (type_spec == NULL) {
        return NULL;
    }
    node_t *node = node_create(ND_DECL_SPEC, type_spec->line, type_spec->col);
    node->decl_spec.type_spec = type_spec;
    return node;
}

node_t *parser_parameter_declaration(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL) {
        return NULL;
    }
    node_t *declarator = parser_declarator(p);
    if (declarator == NULL) {
        node_destroy(decl_spec);
        return NULL;
    }
    node_t *n = node_create(ND_PARAM_DECL, decl_spec->line, decl_spec->col);
    n->param_decl.decl_spec = decl_spec;
    n->param_decl.declarator = declarator;
    return n;
}

node_t *parser_parameter_list(parser_t *p)
{
    node_t *param_decl = parser_parameter_declaration(p);
    if (param_decl == NULL)
        return NULL;

    node_t *n = node_create(ND_PARAM_LIST, param_decl->line, param_decl->col);
    n->param_list.params[0] = param_decl;

    int nparams = 1;
    while (parser_accept(p, ',')) {
        param_decl = parser_parameter_declaration(p);
        if (param_decl == NULL)
            return NULL;
        n->param_list.params[nparams++] = param_decl;
    }
    n->param_list.nparams = nparams;
    return n;
}

node_t *parser_direct_declarator(parser_t *p)
{
    // Identifier
    token_t *ident = parser_expect_token(p, TOKEN_IDENT);
    if (ident == NULL) {
        return NULL;
    }

    node_t *n = node_create(ND_DIRECT_DECL, ident->line, ident->col);
    n->direct_decl.ident.str = ident->sval;
    n->direct_decl.ident.len = ident->len;
    n->direct_decl.param_list = NULL;

    // Declarator
    if (parser_accept(p, '(')) {
        if (parser_accept(p, ')'))
            return n;
        node_t *param_list = parser_parameter_list(p);
        if (!param_list) {
            node_destroy(n);
            return NULL;
        }
        if (!parser_expect(p, ')')) {
            node_destroy(n);
            return NULL;
        }
        n->direct_decl.param_list = param_list;
    }
    return n;
}

bool parser_pointer(parser_t *p)
{
    if (!parser_expect(p, '*'))
        return false;
    while (1) {
        if (!parser_accept(p, '*'))
            return true;
    }
    return true;
}

node_t *parser_declarator(parser_t *p)
{
    if (parser_peek(p)->type == '*') {
        if (!parser_pointer(p)) {
            return NULL;
        }
        // TODO
    }
    node_t *direct_decl = parser_direct_declarator(p);
    if (direct_decl == NULL)
        return NULL;
    return direct_decl;
}

bool parser_expression(parser_t *p)
{
    // TODO
    if (!parser_expect(p, TOKEN_NUM)) {
        return false;
    }
    return true;
}

bool parser_expression_statement(parser_t *p)
{
    if (parser_accept(p, ';')) {
        return true;
    }
    if (!parser_expression(p)) {
        return false;
    }
    if (!parser_expect(p, ';')) {
        return false;
    }
    return true;
}

bool parser_jump_statement(parser_t *p)
{
    if (!parser_expect(p, TOKEN_KW_RETURN)) {
        if (parser_accept(p, ';')) {
            return true;
        }
        if (!parser_expression(p)) {
            return false;
        }
        if (!parser_expect(p, ';')) {
            return false;
        }
    }
    return true;
}

bool parser_statement(parser_t *p)
{
    token_t *t = parser_peek(p);
    if (t->type == '{') {
        if (!parser_compound_statement(p))
            return false;
    } else if (t->type == TOKEN_KW_IF) {
        // TODO: selection statement
    } else if (t->type == TOKEN_KW_WHILE) {
        // TODO: iteration statement
    } else if (t->type == TOKEN_KW_RETURN) {
        if (!parser_jump_statement(p))
            return false;
    } else {
        if (!parser_expression_statement(p))
            return false;
    }
    return true;
}

bool parser_statement_list(parser_t *p)
{
    if (!parser_statement(p))
        return false;
    while (1) {
        if (parser_peek(p)->type == '}') {
            return true;
        }
        if (!parser_statement(p))
            return false;
    }
    return true;
}

bool parser_compound_statement(parser_t *p)
{
    if (!parser_expect(p, '{'))
        return false;
    if (!parser_statement_list(p))
        return false;
    if (!parser_expect(p, '}'))
        return false;
    return true;
}

node_t *parser_function_definition(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL) {
        return NULL;
    }
    node_t *declarator = parser_declarator(p);
    if (declarator == NULL) {
        node_destroy(decl_spec);
        return NULL;
    }
#if 1
    if (!parser_compound_statement(p))
        return NULL;
#else
    node_t *comp_stmt = parser_compound_statement(p);
    if (ncs == NULL) {
        node_destroy(decl_spec);
        node_destroy(declarator);
        return NULL;
    }
#endif
    node_t *node = node_create(ND_FUNC, decl_spec->line, decl_spec->col);
    node->func.decl_spec = decl_spec;
    node->func.declarator = declarator;
    node->func.comp_stmt = NULL;
    return node;
}

bool parser_translation_unit(parser_t *p)
{
    node_t *func = parser_function_definition(p);
    if (func == NULL) {
        return false;
    }
    ast_print(func, 0);
    return true;
}

int parser_exec(parser_t *p)
{
    if (!parser_translation_unit(p)) {
        return -1;
    }
    return 0;
}
