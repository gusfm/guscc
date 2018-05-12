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
        case TOKEN_SEMICOLON:
            return ";";
        case TOKEN_OPEN_BRACE:
            return "{";
        case TOKEN_CLOSE_BRACE:
            return "}";
        case TOKEN_NUMBER:
            return "number";
        case TOKEN_KW_AUTO:
            return "auto";
        case TOKEN_KW_BREAK:
            return "break";
        case TOKEN_KW_CASE:
            return "case";
        case TOKEN_KW_CHAR:
            return "char";
        case TOKEN_KW_CONST:
            return "const";
        case TOKEN_KW_CONTINUE:
            return "continue";
        case TOKEN_KW_DEFAULT:
            return "default";
        case TOKEN_KW_DO:
            return "do";
        case TOKEN_KW_DOUBLE:
            return "double";
        case TOKEN_KW_ELSE:
            return "else";
        case TOKEN_KW_ENUM:
            return "enum";
        case TOKEN_KW_EXTERN:
            return "extern";
        case TOKEN_KW_FLOAT:
            return "float";
        case TOKEN_KW_FOR:
            return "for";
        case TOKEN_KW_GOTO:
            return "goto";
        case TOKEN_KW_IF:
            return "if";
        case TOKEN_KW_INT:
            return "int";
        case TOKEN_KW_LONG:
            return "long";
        case TOKEN_KW_REGISTER:
            return "register";
        case TOKEN_KW_RETURN:
            return "return";
        case TOKEN_KW_SHORT:
            return "short";
        case TOKEN_KW_SIGNED:
            return "signed";
        case TOKEN_KW_SIZEOF:
            return "sizeof";
        case TOKEN_KW_STATIC:
            return "static";
        case TOKEN_KW_STRUCT:
            return "struct";
        case TOKEN_KW_SWITCH:
            return "switch";
        case TOKEN_KW_TYPEDEF:
            return "typedef";
        case TOKEN_KW_UNION:
            return "union";
        case TOKEN_KW_UNSIGNED:
            return "unsigned";
        case TOKEN_KW_VOID:
            return "void";
        case TOKEN_KW_VOLATILE:
            return "volatile";
        case TOKEN_KW_WHILE:
            return "while";
        default:
            return NULL;
    }
}
