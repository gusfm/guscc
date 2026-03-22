// guscc: a simple recursive C compiler
// It will support a limited number of features. The minimum necessary for
// self-hosting.
// The code will be contained in this single file, so that it simplifies the
// compiler and it is easy for beginners to understand.
// It will generate AST nodes and later process them to generate code.
// Features:  enum, struct, functions, if, while
// References:
// - chibicc

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "lex.h"
#include "parser.h"

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

    lex_init(&lex, buf, size);

    while ((t = lex_next(&lex)) != NULL) {
        token_print(t);
        token_destroy(t);
    }
    printf("\n");

    return 0;
}

node_t *parser(char *buf, int size)
{
    parser_t p;

    parser_init(&p, buf, size);
    node_t *ast = parser_exec(&p);
    parser_finish(&p);
    return ast;
}

int codegen(node_t *ast, const char *outpath)
{
    FILE *out = fopen(outpath, "w");
    if (!out) {
        perror("fopen");
        return -1;
    }
    codegen_t cg;
    codegen_init(&cg, out);
    int ret = codegen_exec(&cg, ast);
    codegen_finish(&cg);
    fclose(out);
    return ret;
}

void debug_file(char *buf)
{
    char *endline, *ptr = buf;
    int line = 1;
    while ((endline = strchr(ptr, '\n')) != NULL) {
        printf("%d: ", line);
        printf("%.*s\n", (int)(endline - ptr), ptr);
        ++line;
        ptr = endline + 1;
    }
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

    printf("File debug output:\n");
    debug_file(buf);

    printf("\nLexer debug output:\n");
    lexer(buf, size);

    char outpath[512];
    const char *base = strrchr(argv[1], '/');
    base = base ? base + 1 : argv[1];
    strncpy(outpath, base, sizeof(outpath) - 1);
    outpath[sizeof(outpath) - 1] = '\0';
    size_t inlen = strlen(outpath);
    if (inlen >= 2 && outpath[inlen - 2] == '.' && outpath[inlen - 1] == 'c')
        outpath[inlen - 1] = 's';
    else
        strncat(outpath, ".s", sizeof(outpath) - inlen - 1);

    printf("\nParser debug output:\n");
    node_t *ast = parser(buf, size);
    if (ast == NULL) {
        free(buf);
        return -1;
    }

    int ret = codegen(ast, outpath);
    node_destroy(ast);
    free(buf);
    return ret;
}
