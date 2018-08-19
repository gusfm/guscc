#include "lex.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "str.h"
#include "token.h"

int line;
int col;

static char lex_readc(lex_t *l)
{
    int c = fgetc(l->input);
    if (c == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    return c;
}

static int lex_ungetc(lex_t *l, int c)
{
    l->col--;
    return ungetc(c, l->input);
}

static void lex_skip_space(lex_t *l)
{
    int c;
    while ((c = lex_readc(l)) != EOF) {
        if (isspace(c)) {
            continue;
        }
        lex_ungetc(l, c);
        return;
    }
}

static token_type_t get_token_type(char *s)
{
    if (strcmp(s, "auto") == 0)
        return TOKEN_KW_AUTO;
    if (strcmp(s, "break") == 0)
        return TOKEN_KW_BREAK;
    if (strcmp(s, "case") == 0)
        return TOKEN_KW_CASE;
    if (strcmp(s, "char") == 0)
        return TOKEN_KW_CHAR;
    if (strcmp(s, "const") == 0)
        return TOKEN_KW_CONST;
    if (strcmp(s, "continue") == 0)
        return TOKEN_KW_CONTINUE;
    if (strcmp(s, "default") == 0)
        return TOKEN_KW_DEFAULT;
    if (strcmp(s, "do") == 0)
        return TOKEN_KW_DO;
    if (strcmp(s, "double") == 0)
        return TOKEN_KW_DOUBLE;
    if (strcmp(s, "else") == 0)
        return TOKEN_KW_ELSE;
    if (strcmp(s, "enum") == 0)
        return TOKEN_KW_ENUM;
    if (strcmp(s, "extern") == 0)
        return TOKEN_KW_EXTERN;
    if (strcmp(s, "float") == 0)
        return TOKEN_KW_FLOAT;
    if (strcmp(s, "for") == 0)
        return TOKEN_KW_FOR;
    if (strcmp(s, "goto") == 0)
        return TOKEN_KW_GOTO;
    if (strcmp(s, "if") == 0)
        return TOKEN_KW_IF;
    if (strcmp(s, "int") == 0)
        return TOKEN_KW_INT;
    if (strcmp(s, "long") == 0)
        return TOKEN_KW_LONG;
    if (strcmp(s, "register") == 0)
        return TOKEN_KW_REGISTER;
    if (strcmp(s, "return") == 0)
        return TOKEN_KW_RETURN;
    if (strcmp(s, "short") == 0)
        return TOKEN_KW_SHORT;
    if (strcmp(s, "signed") == 0)
        return TOKEN_KW_SIGNED;
    if (strcmp(s, "sizeof") == 0)
        return TOKEN_KW_SIZEOF;
    if (strcmp(s, "static") == 0)
        return TOKEN_KW_STATIC;
    if (strcmp(s, "struct") == 0)
        return TOKEN_KW_STRUCT;
    if (strcmp(s, "switch") == 0)
        return TOKEN_KW_SWITCH;
    if (strcmp(s, "typedef") == 0)
        return TOKEN_KW_TYPEDEF;
    if (strcmp(s, "union") == 0)
        return TOKEN_KW_UNION;
    if (strcmp(s, "unsigned") == 0)
        return TOKEN_KW_UNSIGNED;
    if (strcmp(s, "void") == 0)
        return TOKEN_KW_VOID;
    if (strcmp(s, "volatile") == 0)
        return TOKEN_KW_VOLATILE;
    if (strcmp(s, "while") == 0)
        return TOKEN_KW_WHILE;
    return TOKEN_IDENT;
}

static token_t *read_ident(lex_t *l, char c)
{
    str_t *str = str_create();
    str_append(str, c);
    for (;;) {
        token_type_t tok_type;
        char *tok_str;
        c = lex_readc(l);
        if (isalnum(c) || c == '_') {
            str_append(str, c);
            continue;
        }
        lex_ungetc(l, c);
        str_append(str, '\0');
        tok_str = str_destroy(str);
        tok_type = get_token_type(tok_str);
        if (tok_type == TOKEN_IDENT) {
            return token_create_string(tok_type, tok_str);
        } else {
            free(tok_str);
            return token_create(tok_type);
        }
    }
}

static token_t *read_number(lex_t *l, char c)
{
    str_t *str = str_create();
    str_append(str, c);
    for (;;) {
        c = lex_readc(l);
        if (isdigit(c)) {
            str_append(str, c);
            continue;
        }
        lex_ungetc(l, c);
        str_append(str, '\0');
        return token_create_string(TOKEN_CONSTANT, str_destroy(str));
    }
}

static token_t *read_less_than(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '<') {
        c = lex_readc(l);
        if (c == '=') {
            return token_create(TOKEN_LSH_ASSIGN);
        } else {
            lex_ungetc(l, c);
            return token_create(TOKEN_LSH_OP);
        }
    } else if (c == '=') {
        return token_create(TOKEN_LE_OP);
    } else {
        lex_ungetc(l, c);
        return token_create('<');
    }
}

static token_t *read_greater_than(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '>') {
        c = lex_readc(l);
        if (c == '=') {
            return token_create(TOKEN_RSH_ASSIGN);
        } else {
            lex_ungetc(l, c);
            return token_create(TOKEN_RSH_OP);
        }
    } else if (c == '=') {
        return token_create(TOKEN_GE_OP);
    } else {
        lex_ungetc(l, c);
        return token_create('>');
    }
}

static token_t *read_add(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '+') {
        return token_create(TOKEN_INC_OP);
    } else if (c == '=') {
        return token_create(TOKEN_ADD_ASSIGN);
    } else {
        lex_ungetc(l, c);
        return token_create('+');
    }
}

static token_t *read_sub(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '-') {
        return token_create(TOKEN_DEC_OP);
    } else if (c == '=') {
        return token_create(TOKEN_SUB_ASSIGN);
    } else if (c == '>') {
        return token_create(TOKEN_PTR_OP);
    } else {
        lex_ungetc(l, c);
        return token_create('-');
    }
}

static token_t *read_asterisk(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '=') {
        return token_create(TOKEN_MUL_ASSIGN);
    } else {
        lex_ungetc(l, c);
        return token_create('*');
    }
}

static token_t *read_div(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '=') {
        return token_create(TOKEN_DIV_ASSIGN);
    } else {
        lex_ungetc(l, c);
        return token_create('/');
    }
}

static token_t *read_mod(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '=') {
        return token_create(TOKEN_MOD_ASSIGN);
    } else {
        lex_ungetc(l, c);
        return token_create('%');
    }
}

static token_t *read_and(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '&') {
        return token_create(TOKEN_AND_OP);
    } else if (c == '=') {
        return token_create(TOKEN_AND_ASSIGN);
    } else {
        lex_ungetc(l, c);
        return token_create('&');
    }
}

static token_t *read_or(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '|') {
        return token_create(TOKEN_OR_OP);
    } else if (c == '=') {
        return token_create(TOKEN_OR_ASSIGN);
    } else {
        lex_ungetc(l, c);
        return token_create('|');
    }
}

static token_t *read_xor(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '=') {
        return token_create(TOKEN_XOR_ASSIGN);
    } else {
        lex_ungetc(l, c);
        return token_create('^');
    }
}

static token_t *read_eq(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '=') {
        return token_create(TOKEN_EQ_OP);
    } else {
        lex_ungetc(l, c);
        return token_create('=');
    }
}

static token_t *read_not(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '=') {
        return token_create(TOKEN_NE_OP);
    } else {
        lex_ungetc(l, c);
        return token_create('!');
    }
}

static char read_escaped_char(lex_t *l)
{
    char c = lex_readc(l);
    switch (c) {
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        default:
        case '\'':
        case '\\':
        case '"':
            return c;
    }
    line = l->line;
    col = l->col;
    ccwarn("unknown escape character: \\%c", c);
    return c;
}

static token_t *read_char(lex_t *l, char c)
{
    c = lex_readc(l);
    if (c == '\\')
        c = read_escaped_char(l);
    lex_readc(l);
    return token_create_char(c);
}

static token_t *read_string(lex_t *l, char c)
{
    str_t *str = str_create();
    for (;;) {
        c = lex_readc(l);
        if (c != '"') {
            str_append(str, c);
            continue;
        }
        str_append(str, '\0');
        return token_create_string(TOKEN_STRING_LITERAL, str_destroy(str));
    }
}

token_t *lex_next_token(lex_t *l)
{
    int c;
    lex_skip_space(l);
    line = l->line;
    col = l->col;
    c = lex_readc(l);
    switch (c) {
        case '"':
            return read_string(l, c);
        case '\'':
            return read_char(l, c);
        case '(':
        case ')':
        case ',':
        case ':':
        case ';':
        case '[':
        case ']':
        case '{':
        case '}':
        case '?':
        case '~':
        case '.':
            return token_create(c);
        case '+':
            return read_add(l, c);
        case '-':
            return read_sub(l, c);
        case '*':
            return read_asterisk(l, c);
        case '/':
            return read_div(l, c);
        case '%':
            return read_mod(l, c);
        case '&':
            return read_and(l, c);
        case '|':
            return read_or(l, c);
        case '^':
            return read_xor(l, c);
        case '=':
            return read_eq(l, c);
        case '!':
            return read_not(l, c);
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
            return read_number(l, c);
        case '<':
            return read_less_than(l, c);
        case '>':
            return read_greater_than(l, c);
        case EOF:
            return NULL;
        default:
            return read_ident(l, c);
    }
}

int lex_init(lex_t *l, FILE *input)
{
    l->input = input;
    l->line = 1;
    l->col = 1;
    return 0;
}
