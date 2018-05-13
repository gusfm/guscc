#include "token.h"
#include <stdlib.h>

extern int line;
extern int col;

token_t *token_create(token_type_t type)
{
    token_t *t = malloc(sizeof(token_t));
    t->type = type;
    t->line = line;
    t->col = col;
    t->val.s = NULL;
    return t;
}

token_t *token_create_char(char c)
{
    token_t *t = token_create(TOKEN_CHAR);
    t->val.c = c;
    return t;
}

token_t *token_create_string(token_type_t type, char *s)
{
    token_t *t = token_create(type);
    t->val.s = s;
    return t;
}

void token_destroy(token_t *t)
{
    if (t->type != TOKEN_CHAR)
        free(t->val.s);
    free(t);
}
