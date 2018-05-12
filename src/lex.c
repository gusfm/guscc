#include "lex.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "str.h"
#include "token.h"

char lex_readc(lex_t *l)
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

int lex_ungetc(lex_t *l, int c)
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
    if (strcmp(s, "int") == 0)
        return TOKEN_KW_INT;
    if (strcmp(s, "return") == 0)
        return TOKEN_KW_RETURN;
    if (strcmp(s, "void") == 0)
        return TOKEN_KW_VOID;
    return TOKEN_IDENT;
}

token_t *read_ident(lex_t *l, char c)
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
        if (tok_type != TOKEN_IDENT) {
            free(tok_str);
            tok_str = NULL;
        }
        return token_create(tok_type, l->tok_line, l->tok_col, tok_str);
    }
}

token_t *read_number(lex_t *l, char c)
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
        return token_create(TOKEN_NUMBER, l->tok_line, l->tok_col,
                            str_destroy(str));
    }
}

token_t *lex_next_token(lex_t *l)
{
    int c;
    lex_skip_space(l);
    l->tok_line = l->line;
    l->tok_col = l->col;
    c = lex_readc(l);
    switch (c) {
        case '(':
            return token_create(TOKEN_OPEN_PAR, l->tok_line, l->tok_col, NULL);
        case ')':
            return token_create(TOKEN_CLOSE_PAR, l->tok_line, l->tok_col, NULL);
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
        case ';':
            return token_create(TOKEN_SEMICOLON, l->tok_line, l->tok_col,
                                NULL);
        case '{':
            return token_create(TOKEN_OPEN_BRACE, l->tok_line, l->tok_col,
                                NULL);
        case '}':
            return token_create(TOKEN_CLOSE_BRACE, l->tok_line, l->tok_col,
                                NULL);
        case EOF:
            return token_create(TOKEN_EOF, -1, -1, NULL);
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

void lex_execute(lex_t *l)
{
    token_t *t;
    printf("tokens:\n");
    do {
        t = lex_next_token(l);
        printf("type=%s, line=%d, col=%d", token_type_str(t->type), t->line,
               t->col);
        if (t->type == TOKEN_IDENT || t->type == TOKEN_NUMBER) {
            printf(", str=\"%s\"", t->sval);
        }
        printf("\n");
        token_destroy(t);
    } while (t->type != TOKEN_EOF);
}
