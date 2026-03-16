#include "lex.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

int tok_line;
int tok_col;

void lex_init(lex_t *l, char *start, size_t size)
{
    l->start = start;
    l->end = start + size;
    l->p = start;
    l->line = 1;
    l->col = 1;
}

static char lex_readc(lex_t *l)
{
    int c = *l->p++;
    if (c == '\0') {
        return EOF;
    } else if (c == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    return c;
}

static void lex_ungetc(lex_t *l)
{
    l->col--;
    l->p--;
}

static token_type_t get_token_type(char *s, int len)
{
    if (strncmp(s, "char", len) == 0)
        return TOKEN_KW_CHAR;
    if (strncmp(s, "if", len) == 0)
        return TOKEN_KW_IF;
    if (strncmp(s, "int", len) == 0)
        return TOKEN_KW_INT;
    if (strncmp(s, "return", len) == 0)
        return TOKEN_KW_RETURN;
    if (strncmp(s, "void", len) == 0)
        return TOKEN_KW_VOID;
    if (strncmp(s, "while", len) == 0)
        return TOKEN_KW_WHILE;
    return TOKEN_IDENT;
}

static token_t *read_number(lex_t *l)
{
    char *s = l->p - 1;
    while (1) {
        char c = lex_readc(l);
        if (isdigit(c)) {
            continue;
        }
        lex_ungetc(l);
        return token_create(TOKEN_NUM, s, l->p, tok_line, tok_col);
    }
}

static token_t *read_ident(lex_t *l)
{
    char *s = l->p - 1;
    while (1) {
        char c = lex_readc(l);
        if (isalnum(c) || c == '_') {
            continue;
        }
        lex_ungetc(l);
        token_type_t tok_type = get_token_type(s, l->p - s);
        return token_create(tok_type, s, l->p, tok_line, tok_col);
    }
}

static token_t *read_string(lex_t *l)
{
    char *s = l->p - 1;
    while (1) {
        char c = lex_readc(l);
        if (c != '"') {
            continue;
        }
        return token_create(TOKEN_STR, s, l->p, tok_line, tok_col);
    }
}

static char lex_next_char(lex_t *l)
{
    int c;
    while ((c = lex_readc(l)) != EOF) {
        if (isspace(c)) {
            continue;
        }
        tok_line = l->line;
        tok_col = l->col - 1;
        return c;
    }
    return EOF;
}

token_t *lex_next(lex_t *l)
{
    int c = lex_next_char(l);
    switch (c) {
        case '"':
            return read_string(l);
        case '(':
        case ')':
        case '*':
        case ',':
        case ';':
        case '[':
        case ']':
        case '{':
        case '}':
            return token_create(c, l->p - 1, l->p, tok_line, tok_col);
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return read_number(l);
        case EOF:
            return NULL;
        default:
            return read_ident(l);
    }
}
