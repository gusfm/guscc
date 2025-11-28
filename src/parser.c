#include "parser.h"

#include <stdbool.h>
#include <stdio.h>

// Foward declarations
bool parser_declarator(parser_t *p);
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
        printf("OK: '%.*s'\n", t->len, t->sval);
        parser_next(p); // consume it
        return true;
    }
    return false;
}

bool parser_expect(parser_t *p, token_type_t type)
{
    int ret;
    token_t *t = parser_next(p);
    if (t->type == type) {
        printf("OK: '%.*s'\n", t->len, t->sval);
        ret = true;
    } else {
        char str[100];
        fprintf(stderr, "Expected %s but received '%.*s'\n",
                token_type_to_str(type, str, 100), t->len, t->sval);
        ret = false;
    }
    token_destroy(t);
    return ret;
}

bool parser_declaration_specifiers(parser_t *p)
{
    int ret = true;
    token_t *t;
    t = parser_next(p);
    if (t->type == TOKEN_KW_INT) {
        printf("INT\n");
    } else if (t->type == TOKEN_KW_CHAR) {
        printf("CHAR\n");
    } else if (t->type == TOKEN_KW_VOID) {
        printf("VOID\n");
    } else {
        fprintf(stderr, "Expected type but received '%.*s'\n", t->len, t->sval);
        ret = false;
    }
    token_destroy(t);
    return ret;
}

bool parser_parameter_declaration(parser_t *p)
{
    if (!parser_declaration_specifiers(p))
        return false;
    if (!parser_declarator(p))
        return false;
    return true;
}

bool parser_parameter_list(parser_t *p)
{
    if (!parser_parameter_declaration(p))
        return false;

    while (parser_accept(p, ',')) {
        if (!parser_parameter_declaration(p))
            return false;
    }
    return true;
}

int parser_direct_declarator(parser_t *p)
{
    // Identifier
    if (!parser_expect(p, TOKEN_IDENT))
        return false;

    // Declarator
    if (parser_accept(p, '(')) {
        if (parser_accept(p, ')'))
            return true;
        if (!parser_parameter_list(p))
            return false;
        if (!parser_expect(p, ')'))
            return false;
        return true;
    }

    return true;
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

bool parser_declarator(parser_t *p)
{
    if (parser_peek(p)->type == '*') {
        parser_pointer(p);
    }
    if (!parser_direct_declarator(p))
        return false;
    return true;
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

bool parser_function_definition(parser_t *p)
{
    if (!parser_declaration_specifiers(p))
        return false;
    if (!parser_declarator(p))
        return false;
    if (!parser_compound_statement(p))
        return false;
    return true;
}

bool parser_translation_unit(parser_t *p)
{
    if (!parser_function_definition(p))
        return false;
    return true;
}

int parser_exec(parser_t *p)
{
    if (!parser_translation_unit(p)) {
        return -1;
    }
    return 0;
}
