#include "lex.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ident.h"
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
    ident_t *ident = ident_create();
    ident_append(ident, c);
    for (;;) {
        token_type_t tok_type;
        char *tok_str;
        c = lex_readc(l);
        if (isalnum(c) || c == '_') {
            ident_append(ident, c);
            continue;
        }
        lex_ungetc(l, c);
        ident_append(ident, '\0');
        tok_str = ident_destroy(ident);
        tok_type = get_token_type(tok_str);
        if (tok_type != TOKEN_IDENT) {
            free(tok_str);
            tok_str = NULL;
        }
        return token_create(tok_type, l->tok_line, l->tok_col, tok_str);
    }
}

static token_t *lex_next_token(lex_t *l)
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
        case ';':
            return token_create(TOKEN_SEMI_COLON, l->tok_line, l->tok_col,
                                NULL);
        case '{':
            return token_create(TOKEN_OPEN_BRACE, l->tok_line, l->tok_col,
                                NULL);
        case '}':
            return token_create(TOKEN_CLOSE_BRACE, l->tok_line, l->tok_col,
                                NULL);
        case EOF:
            return NULL;
        default:
            return read_ident(l, c);
    }
}

int lex_init(lex_t *l, const char *filename)
{
    l->input = fopen(filename, "r");
    if (l->input == NULL) {
        fprintf(stderr, "error: could not open %s\n", filename);
        return -1;
    }
    l->filename = filename;
    l->line = 1;
    l->col = 1;
    return 0;
}

void lex_finish(lex_t *l)
{
    fclose(l->input);
}

void lex_execute(lex_t *l)
{
    token_t *t;
    printf("tokens:\n");
    while ((t = lex_next_token(l)) != NULL) {
        printf("type=%s, line=%d, col=%d", token_type_str(t->type), t->line,
               t->col);
        if (t->type == TOKEN_IDENT) {
            printf(", ident=\"%s\"", t->sval);
        }
        printf("\n");
        token_destroy(t);
    }
}
