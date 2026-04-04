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
    if (len == 4 && strncmp(s, "char", 4) == 0)
        return TOKEN_KW_CHAR;
    if (len == 4 && strncmp(s, "else", 4) == 0)
        return TOKEN_KW_ELSE;
    if (len == 2 && strncmp(s, "if", 2) == 0)
        return TOKEN_KW_IF;
    if (len == 3 && strncmp(s, "int", 3) == 0)
        return TOKEN_KW_INT;
    if (len == 6 && strncmp(s, "return", 6) == 0)
        return TOKEN_KW_RETURN;
    if (len == 4 && strncmp(s, "void", 4) == 0)
        return TOKEN_KW_VOID;
    if (len == 5 && strncmp(s, "while", 5) == 0)
        return TOKEN_KW_WHILE;
    if (len == 6 && strncmp(s, "sizeof", 6) == 0)
        return TOKEN_KW_SIZEOF;
    if (len == 5 && strncmp(s, "break", 5) == 0)
        return TOKEN_KW_BREAK;
    if (len == 8 && strncmp(s, "continue", 8) == 0)
        return TOKEN_KW_CONTINUE;
    if (len == 2 && strncmp(s, "do", 2) == 0)
        return TOKEN_KW_DO;
    if (len == 3 && strncmp(s, "for", 3) == 0)
        return TOKEN_KW_FOR;
    if (len == 6 && strncmp(s, "struct", 6) == 0)
        return TOKEN_KW_STRUCT;
    if (len == 6 && strncmp(s, "switch", 6) == 0)
        return TOKEN_KW_SWITCH;
    if (len == 4 && strncmp(s, "case", 4) == 0)
        return TOKEN_KW_CASE;
    if (len == 7 && strncmp(s, "default", 7) == 0)
        return TOKEN_KW_DEFAULT;
    if (len == 4 && strncmp(s, "enum", 4) == 0)
        return TOKEN_KW_ENUM;
    if (len == 5 && strncmp(s, "short", 5) == 0)
        return TOKEN_KW_SHORT;
    if (len == 4 && strncmp(s, "long", 4) == 0)
        return TOKEN_KW_LONG;
    if (len == 6 && strncmp(s, "static", 6) == 0)
        return TOKEN_KW_STATIC;
    if (len == 6 && strncmp(s, "extern", 6) == 0)
        return TOKEN_KW_EXTERN;
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
    char c2, c3;
    switch (c) {
        case '"':
            return read_string(l);
        case '(':
        case ')':
        case ',':
        case ';':
        case '[':
        case ']':
        case '{':
        case '}':
        case '~':
        case '?':
        case ':':
            return token_create(c, l->p - 1, l->p, tok_line, tok_col);
        case '.':
            c2 = lex_readc(l);
            if (c2 == '.') {
                c3 = lex_readc(l);
                if (c3 == '.')
                    return token_create(TOKEN_ELLIPSIS, l->p - 3, l->p, tok_line, tok_col);
                lex_ungetc(l);
            }
            lex_ungetc(l);
            return token_create('.', l->p - 1, l->p, tok_line, tok_col);
        case '+':
            c2 = lex_readc(l);
            if (c2 == '+')
                return token_create(TOKEN_INC_OP, l->p - 2, l->p, tok_line, tok_col);
            if (c2 == '=')
                return token_create(TOKEN_ADD_ASSIGN, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('+', l->p - 1, l->p, tok_line, tok_col);
        case '-':
            c2 = lex_readc(l);
            if (c2 == '-')
                return token_create(TOKEN_DEC_OP, l->p - 2, l->p, tok_line, tok_col);
            if (c2 == '>')
                return token_create(TOKEN_PTR_OP, l->p - 2, l->p, tok_line, tok_col);
            if (c2 == '=')
                return token_create(TOKEN_SUB_ASSIGN, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('-', l->p - 1, l->p, tok_line, tok_col);
        case '*':
            c2 = lex_readc(l);
            if (c2 == '=')
                return token_create(TOKEN_MUL_ASSIGN, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('*', l->p - 1, l->p, tok_line, tok_col);
        case '/':
            c2 = lex_readc(l);
            if (c2 == '=')
                return token_create(TOKEN_DIV_ASSIGN, l->p - 2, l->p, tok_line, tok_col);
            if (c2 == '/') {
                while ((c2 = lex_readc(l)) != '\n' && c2 != EOF)
                    ;
                return lex_next(l);
            }
            if (c2 == '*') {
                while (1) {
                    c2 = lex_readc(l);
                    if (c2 == EOF)
                        return NULL;
                    if (c2 == '*') {
                        c3 = lex_readc(l);
                        if (c3 == '/')
                            break;
                        lex_ungetc(l);
                    }
                }
                return lex_next(l);
            }
            lex_ungetc(l);
            return token_create('/', l->p - 1, l->p, tok_line, tok_col);
        case '%':
            c2 = lex_readc(l);
            if (c2 == '=')
                return token_create(TOKEN_MOD_ASSIGN, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('%', l->p - 1, l->p, tok_line, tok_col);
        case '<':
            c2 = lex_readc(l);
            if (c2 == '<') {
                c3 = lex_readc(l);
                if (c3 == '=')
                    return token_create(TOKEN_LEFT_ASSIGN, l->p - 3, l->p, tok_line, tok_col);
                lex_ungetc(l);
                return token_create(TOKEN_LEFT_OP, l->p - 2, l->p, tok_line, tok_col);
            }
            if (c2 == '=')
                return token_create(TOKEN_LE_OP, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('<', l->p - 1, l->p, tok_line, tok_col);
        case '>':
            c2 = lex_readc(l);
            if (c2 == '>') {
                c3 = lex_readc(l);
                if (c3 == '=')
                    return token_create(TOKEN_RIGHT_ASSIGN, l->p - 3, l->p, tok_line, tok_col);
                lex_ungetc(l);
                return token_create(TOKEN_RIGHT_OP, l->p - 2, l->p, tok_line, tok_col);
            }
            if (c2 == '=')
                return token_create(TOKEN_GE_OP, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('>', l->p - 1, l->p, tok_line, tok_col);
        case '=':
            c2 = lex_readc(l);
            if (c2 == '=')
                return token_create(TOKEN_EQ_OP, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('=', l->p - 1, l->p, tok_line, tok_col);
        case '!':
            c2 = lex_readc(l);
            if (c2 == '=')
                return token_create(TOKEN_NE_OP, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('!', l->p - 1, l->p, tok_line, tok_col);
        case '&':
            c2 = lex_readc(l);
            if (c2 == '&')
                return token_create(TOKEN_AND_OP, l->p - 2, l->p, tok_line, tok_col);
            if (c2 == '=')
                return token_create(TOKEN_AND_ASSIGN, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('&', l->p - 1, l->p, tok_line, tok_col);
        case '|':
            c2 = lex_readc(l);
            if (c2 == '|')
                return token_create(TOKEN_OR_OP, l->p - 2, l->p, tok_line, tok_col);
            if (c2 == '=')
                return token_create(TOKEN_OR_ASSIGN, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('|', l->p - 1, l->p, tok_line, tok_col);
        case '^':
            c2 = lex_readc(l);
            if (c2 == '=')
                return token_create(TOKEN_XOR_ASSIGN, l->p - 2, l->p, tok_line, tok_col);
            lex_ungetc(l);
            return token_create('^', l->p - 1, l->p, tok_line, tok_col);
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
