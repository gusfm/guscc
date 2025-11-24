// guscc: a simple recursive C compiler
// It will support a limited number of features. The minimum necessary for
// self-hosting.
// The code will be contained in this single file, so that it simplifies the
// compiler and it is easy for beginners to understand.
// It will generate AST nodes and later process them to generate code.
// Features:  enum, struct, functions, if, while
// References:
// - chibicc

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOKEN_IDENT = 0x100,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_KW_INT,
    TOKEN_KW_CHAR,
    TOKEN_KW_RETURN,
    TOKEN_KW_VOID,
    TOKEN_EOF
} token_type_t;

typedef struct {
    token_type_t type; // Token type
    char *sval;        // String literal (not null terminated)
    int len;           // Token length
} token_t;

typedef struct {
    char *start; // Start of file loaded to string
    char *end;   // End of file loaded to string
    char *p;     // Pointer to current file position
    int line;    // Line number of file
    int col;     // Column number of file
} lex_t;

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
    } else if (type == TOKEN_KW_INT) {
        printf("INT ");
    } else if (type == TOKEN_KW_CHAR) {
        printf("CHAR ");
    } else if (type == TOKEN_KW_RETURN) {
        printf("RETURN ");
    } else if (type == TOKEN_KW_VOID) {
        printf("VOID ");
    } else if (type == TOKEN_EOF)
        printf("EOF");
    else
        printf("\ninvalid token %d\n", t->type);
}

int lex_init(lex_t *l, char *start, char *end)
{
    l->start = start;
    l->end = end;
    l->p = start;
    l->line = 1;
    l->col = 1;
    return 0;
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
    if (strncmp(s, "int", len) == 0)
        return TOKEN_KW_INT;
    if (strncmp(s, "char", len) == 0)
        return TOKEN_KW_CHAR;
    if (strncmp(s, "return", len) == 0)
        return TOKEN_KW_RETURN;
    if (strncmp(s, "void", len) == 0)
        return TOKEN_KW_VOID;
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
        return token_create(TOKEN_NUM, s, l->p);
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
        return token_create(tok_type, s, l->p);
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
        return token_create(TOKEN_STR, s, l->p);
    }
}

static char lex_next_char(lex_t *l)
{
    int c;
    while ((c = lex_readc(l)) != EOF) {
        if (isspace(c)) {
            continue;
        }
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
            return token_create(c, l->p - 1, l->p);
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

char *load_file_to_string(const char *filename, long *length)
{
    FILE *f = fopen(filename, "rb");
    char *buffer;
    if (!f) {
        perror("fopen");
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return NULL;
    }
    long len = ftell(f);
    if (len < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    rewind(f);
    buffer = malloc(len + 1);
    if (!buffer) {
        perror("malloc");
        fclose(f);
        return NULL;
    }
    if (fread(buffer, 1, len, f) != (size_t)len) {
        perror("fread");
        free(buffer);
        fclose(f);
        return NULL;
    }
    buffer[len] = '\0';
    *length = len;
    fclose(f);
    return buffer;
}

int lexer(const char *filename, FILE *out)
{
    lex_t lex;
    token_t *t;
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "error: could not open %s\n", filename);
        return -1;
    }

    long length;
    char *buf = load_file_to_string(filename, &length);

    lex_init(&lex, buf, buf + length);

    while ((t = lex_next(&lex)) != NULL) {
        token_print(t);
        token_destroy(t);
    }
    fprintf(out, "\n");

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "error: syntax: guscc <file.c>\n");
        return -1;
    }
    lexer(argv[1], stdout);
    return 0;
}
