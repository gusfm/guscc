#include "token.h"
#include <stdlib.h>

token_t *token_create(token_type_t type, int line, int col, char *s)
{
    token_t *t = malloc(sizeof(token_t));
    t->type = type;
    t->line = line;
    t->col = col;
    t->sval = s;
    return t;
}

void token_destroy(token_t *t)
{
    if (t->type == TOKEN_IDENT)
        free(t->sval);
    free(t);
}

const char *token_type_str(token_type_t type)
{
    switch (type) {
        case TOKEN_IDENT:
            return "identifier";
        case TOKEN_OPEN_PAR:
            return "(";
        case TOKEN_CLOSE_PAR:
            return ")";
        case TOKEN_SEMI_COLON:
            return ";";
        case TOKEN_OPEN_BRACE:
            return "{";
        case TOKEN_CLOSE_BRACE:
            return "}";
        case TOKEN_KW_INT:
            return "int";
        case TOKEN_KW_RETURN:
            return "return";
        case TOKEN_KW_VOID:
            return "void";
        default:
            return NULL;
    }
}
