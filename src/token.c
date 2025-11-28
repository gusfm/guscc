#include "token.h"

#include <stdio.h>
#include <stdlib.h>

token_t *token_create(token_type_t type, char *start, char *end)
{
    token_t *t = malloc(sizeof(token_t));
    t->type = type;
    t->sval = start;
    t->len = end - start;
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
    } else if (type == TOKEN_KW_IF) {
        printf("IF ");
    } else if (type == TOKEN_KW_INT) {
        printf("INT ");
    } else if (type == TOKEN_KW_RETURN) {
        printf("RETURN ");
    } else if (type == TOKEN_KW_VOID) {
        printf("VOID ");
    } else if (type == TOKEN_KW_WHILE) {
        printf("WHILE ");
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
    } else {
        snprintf(str, len, "not implemented");
    }
    return str;
}
