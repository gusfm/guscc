#include "token.h"

#include <stdio.h>
#include <stdlib.h>

token_t *token_create(token_type_t type, char *start, char *end, int line, int col)
{
    token_t *t = malloc(sizeof(token_t));
    t->type = type;
    t->sval = start;
    t->len = end - start;
    t->line = line;
    t->col = col;
    return t;
}

void token_destroy(token_t *t)
{
    free(t);
}

void token_print(token_t *t)
{
    token_type_t type = t->type;
    if (type > 32 && type < 127) {
        printf("%c ", *t->sval);
    } else if (type == TOKEN_IDENT || type == TOKEN_STR || type == TOKEN_NUM) {
        printf("%.*s ", t->len, t->sval);
    } else if (type == TOKEN_KW_CHAR) {
        printf("CHAR ");
    } else if (type == TOKEN_KW_ELSE) {
        printf("ELSE ");
    } else if (type == TOKEN_KW_IF) {
        printf("IF ");
    } else if (type == TOKEN_KW_INT) {
        printf("INT ");
    } else if (type == TOKEN_KW_RETURN) {
        printf("RETURN ");
    } else if (type == TOKEN_KW_VOID) {
        printf("VOID ");
    } else if (type == TOKEN_KW_BREAK) {
        printf("BREAK ");
    } else if (type == TOKEN_KW_CONTINUE) {
        printf("CONTINUE ");
    } else if (type == TOKEN_KW_DO) {
        printf("DO ");
    } else if (type == TOKEN_KW_FOR) {
        printf("FOR ");
    } else if (type == TOKEN_KW_WHILE) {
        printf("WHILE ");
    } else if (type == TOKEN_KW_SIZEOF) {
        printf("SIZEOF ");
    } else if (type == TOKEN_INC_OP) {
        printf("++ ");
    } else if (type == TOKEN_DEC_OP) {
        printf("-- ");
    } else if (type == TOKEN_PTR_OP) {
        printf("-> ");
    } else if (type == TOKEN_LEFT_OP) {
        printf("<< ");
    } else if (type == TOKEN_RIGHT_OP) {
        printf(">> ");
    } else if (type == TOKEN_LE_OP) {
        printf("<= ");
    } else if (type == TOKEN_GE_OP) {
        printf(">= ");
    } else if (type == TOKEN_EQ_OP) {
        printf("== ");
    } else if (type == TOKEN_NE_OP) {
        printf("!= ");
    } else if (type == TOKEN_AND_OP) {
        printf("&& ");
    } else if (type == TOKEN_OR_OP) {
        printf("|| ");
    } else if (type == TOKEN_MUL_ASSIGN) {
        printf("*= ");
    } else if (type == TOKEN_DIV_ASSIGN) {
        printf("/= ");
    } else if (type == TOKEN_MOD_ASSIGN) {
        printf("%%= ");
    } else if (type == TOKEN_ADD_ASSIGN) {
        printf("+= ");
    } else if (type == TOKEN_SUB_ASSIGN) {
        printf("-= ");
    } else if (type == TOKEN_LEFT_ASSIGN) {
        printf("<<= ");
    } else if (type == TOKEN_RIGHT_ASSIGN) {
        printf(">>= ");
    } else if (type == TOKEN_AND_ASSIGN) {
        printf("&= ");
    } else if (type == TOKEN_XOR_ASSIGN) {
        printf("^= ");
    } else if (type == TOKEN_OR_ASSIGN) {
        printf("|= ");
    } else if (type == TOKEN_EOF)
        printf("EOF");
    else
        printf("\ninvalid token %d\n", t->type);
}

const char *token_type_to_str(token_type_t type, char *str, size_t len)
{
    if (type > 32 && type < 127) {
        snprintf(str, len, "'%c'", (char)type);
    } else if (type == TOKEN_IDENT) {
        snprintf(str, len, "identifier");
    } else if (type == TOKEN_NUM) {
        snprintf(str, len, "constant");
    } else if (type == TOKEN_KW_DO) {
        snprintf(str, len, "do");
    } else if (type == TOKEN_KW_FOR) {
        snprintf(str, len, "for");
    } else if (type == TOKEN_KW_SIZEOF) {
        snprintf(str, len, "sizeof");
    } else if (type == TOKEN_INC_OP) {
        snprintf(str, len, "'++'");
    } else if (type == TOKEN_DEC_OP) {
        snprintf(str, len, "'--'");
    } else if (type == TOKEN_PTR_OP) {
        snprintf(str, len, "'->'");
    } else if (type == TOKEN_LEFT_OP) {
        snprintf(str, len, "'<<'");
    } else if (type == TOKEN_RIGHT_OP) {
        snprintf(str, len, "'>>'");
    } else if (type == TOKEN_LE_OP) {
        snprintf(str, len, "'<='");
    } else if (type == TOKEN_GE_OP) {
        snprintf(str, len, "'>='");
    } else if (type == TOKEN_EQ_OP) {
        snprintf(str, len, "'=='");
    } else if (type == TOKEN_NE_OP) {
        snprintf(str, len, "'!='");
    } else if (type == TOKEN_AND_OP) {
        snprintf(str, len, "'&&'");
    } else if (type == TOKEN_OR_OP) {
        snprintf(str, len, "'||'");
    } else if (type == TOKEN_ADD_ASSIGN) {
        snprintf(str, len, "'+='");
    } else if (type == TOKEN_SUB_ASSIGN) {
        snprintf(str, len, "'-='");
    } else if (type == TOKEN_MUL_ASSIGN) {
        snprintf(str, len, "'*='");
    } else if (type == TOKEN_DIV_ASSIGN) {
        snprintf(str, len, "'/='");
    } else if (type == TOKEN_MOD_ASSIGN) {
        snprintf(str, len, "'%%='");
    } else if (type == TOKEN_LEFT_ASSIGN) {
        snprintf(str, len, "'<<='");
    } else if (type == TOKEN_RIGHT_ASSIGN) {
        snprintf(str, len, "'>>='");
    } else if (type == TOKEN_AND_ASSIGN) {
        snprintf(str, len, "'&='");
    } else if (type == TOKEN_XOR_ASSIGN) {
        snprintf(str, len, "'^='");
    } else if (type == TOKEN_OR_ASSIGN) {
        snprintf(str, len, "'|='");
    } else {
        snprintf(str, len, "not implemented");
    }
    return str;
}

void token_print_error(token_t *t, const char *expected)
{
    fprintf(stderr, "Expected %s but received '%.*s'\n", expected, t->len, t->sval);
}
