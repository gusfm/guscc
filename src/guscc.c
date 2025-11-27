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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOKEN_IDENT = 0x100,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_KW_CHAR,
    TOKEN_KW_IF,
    TOKEN_KW_INT,
    TOKEN_KW_RETURN,
    TOKEN_KW_VOID,
    TOKEN_KW_WHILE,
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

typedef struct {
    lex_t l;             // Lexer
    token_t *next_token; // Cached lookahead token (NULL = not loaded)
} parser_t;

// Foward declarations
bool parser_declarator(parser_t *p);
bool parser_compound_statement(parser_t *p);

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

void lex_init(lex_t *l, char *start, char *end)
{
    l->start = start;
    l->end = end;
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

char *load_file_to_string(const char *filename, long *size)
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
    *size = len;
    fclose(f);
    return buffer;
}

int lexer(char *buf, int size)
{
    lex_t lex;
    token_t *t;

    lex_init(&lex, buf, buf + size);

    while ((t = lex_next(&lex)) != NULL) {
        token_print(t);
        token_destroy(t);
    }
    printf("\n");

    return 0;
}

void parser_init(parser_t *p, char *buf, size_t size)
{
    lex_init(&p->l, buf, buf + size);
    p->next_token = NULL;
}

void parser_finish(parser_t *p)
{
    if (p->next_token) {
        token_destroy(p->next_token);
        p->next_token = NULL;
    }
}

token_t *parser_peek(parser_t *p)
{
    if (p->next_token == NULL) {
        p->next_token = lex_next(&p->l);
    }
    return p->next_token;
}

token_t *parser_next(parser_t *p)
{
    if (p->next_token != NULL) {
        token_t *t = p->next_token;
        p->next_token = NULL;
        return t;
    }
    return lex_next(&p->l);
}

bool parser_accept(parser_t *p, token_type_t type)
{
    token_t *t = parser_peek(p);
    if (t->type == type) {
        printf("OK: '%.*s'\n", t->len, t->sval);
        parser_next(p); // consume it
        return true;
    }
    return false;
}

bool parser_expect(parser_t *p, token_type_t type)
{
    int ret;
    token_t *t = parser_next(p);
    if (t->type == type) {
        printf("OK: '%.*s'\n", t->len, t->sval);
        ret = true;
    } else {
        char str[100];
        fprintf(stderr, "Expected %s but received '%.*s'\n",
                token_type_to_str(type, str, 100), t->len, t->sval);
        ret = false;
    }
    token_destroy(t);
    return ret;
}

bool parser_declaration_specifiers(parser_t *p)
{
    int ret = true;
    token_t *t;
    t = parser_next(p);
    if (t->type == TOKEN_KW_INT) {
        printf("INT\n");
    } else if (t->type == TOKEN_KW_CHAR) {
        printf("CHAR\n");
    } else if (t->type == TOKEN_KW_VOID) {
        printf("VOID\n");
    } else {
        fprintf(stderr, "Expected type but received '%.*s'\n", t->len, t->sval);
        ret = false;
    }
    token_destroy(t);
    return ret;
}

bool parser_parameter_declaration(parser_t *p)
{
    if (!parser_declaration_specifiers(p))
        return false;
    if (!parser_declarator(p))
        return false;
    return true;
}

bool parser_parameter_list(parser_t *p)
{
    if (!parser_parameter_declaration(p))
        return false;

    while (parser_accept(p, ',')) {
        if (!parser_parameter_declaration(p))
            return false;
    }
    return true;
}

int parser_direct_declarator(parser_t *p)
{
    // Identifier
    if (!parser_expect(p, TOKEN_IDENT))
        return false;

    // Declarator
    if (parser_accept(p, '(')) {
        if (parser_accept(p, ')'))
            return true;
        if (!parser_parameter_list(p))
            return false;
        if (!parser_expect(p, ')'))
            return false;
        return true;
    }

    return true;
}

bool parser_pointer(parser_t *p)
{
    if (!parser_expect(p, '*'))
        return false;
    while (1) {
        if (!parser_accept(p, '*'))
            return true;
    }
    return true;
}

bool parser_declarator(parser_t *p)
{
    if (parser_peek(p)->type == '*') {
        parser_pointer(p);
    }
    if (!parser_direct_declarator(p))
        return false;
    return true;
}

bool parser_expression(parser_t *p)
{
    // TODO
    if (!parser_expect(p, TOKEN_NUM)) {
        return false;
    }
    return true;
}

bool parser_expression_statement(parser_t *p)
{
    if (parser_accept(p, ';')) {
        return true;
    }
    if (!parser_expression(p)) {
        return false;
    }
    if (!parser_expect(p, ';')) {
        return false;
    }
    return true;
}

bool parser_jump_statement(parser_t *p)
{
    if (!parser_expect(p, TOKEN_KW_RETURN)) {
        if (parser_accept(p, ';')) {
            return true;
        }
        if (!parser_expression(p)) {
            return false;
        }
        if (!parser_expect(p, ';')) {
            return false;
        }
    }
    return true;
}

bool parser_statement(parser_t *p)
{
    token_t *t = parser_peek(p);
    if (t->type == '{') {
        if (!parser_compound_statement(p))
            return false;
    } else if (t->type == TOKEN_KW_IF) {
        // TODO: selection statement
    } else if (t->type == TOKEN_KW_WHILE) {
        // TODO: iteration statement
    } else if (t->type == TOKEN_KW_RETURN) {
        if (!parser_jump_statement(p))
            return false;
    } else {
        if (!parser_expression_statement(p))
            return false;
    }
    return true;
}

bool parser_statement_list(parser_t *p)
{
    if (!parser_statement(p))
        return false;
    while (1) {
        if (parser_peek(p)->type == '}') {
            return true;
        }
        if (!parser_statement(p))
            return false;
    }
    return true;
}

bool parser_compound_statement(parser_t *p)
{
    if (!parser_expect(p, '{'))
        return false;
    if (!parser_statement_list(p))
        return false;
    if (!parser_expect(p, '}'))
        return false;
    return true;
}

bool parser_function_definition(parser_t *p)
{
    if (!parser_declaration_specifiers(p))
        return false;
    if (!parser_declarator(p))
        return false;
    if (!parser_compound_statement(p))
        return false;
    return true;
}

int parser(char *buf, int size)
{
    parser_t p;

    parser_init(&p, buf, size);
    parser_function_definition(&p);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "error: syntax: guscc <file.c>\n");
        return -1;
    }

    long size;
    char *buf = load_file_to_string(argv[1], &size);
    if (buf == NULL) {
        fprintf(stderr, "error: could not open %s\n", argv[1]);
        return -1;
    }

    printf("Lexer debug output:\n");
    lexer(buf, size);

    printf("Parser debug output:\n");
    parser(buf, size);
    return 0;
}
