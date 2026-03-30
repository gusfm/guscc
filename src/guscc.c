#define _POSIX_C_SOURCE 200809L
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
#include <unistd.h>

#include "ast.h"
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

int guscc(int argc, char **argv)
{
    int debug = 0;
    int asm_only = 0;
    const char *filename = NULL;
    const char *outname = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)
            debug = 1;
        else if (strcmp(argv[i], "-S") == 0)
            asm_only = 1;
        else if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "error: -o requires an argument\n");
                return -1;
            }
            outname = argv[i];
        } else if (filename == NULL)
            filename = argv[i];
        else {
            fprintf(stderr, "error: unexpected argument '%s'\n", argv[i]);
            return -1;
        }
    }

    if (filename == NULL) {
        fprintf(stderr, "error: syntax: guscc [-d] [-S] [-o <output>] <file.c>\n");
        return -1;
    }

    long size;
    char *buf = load_file_to_string(filename, &size);
    if (buf == NULL) {
        fprintf(stderr, "error: could not open %s\n", filename);
        return -1;
    }

    if (debug) {
        printf("File debug output:\n");
        debug_file(buf);
        printf("\nLexer debug output:\n");
        lexer(buf, size);
    }

    /* Derive output path for assembly-only mode */
    char outpath[512];
    if (asm_only) {
        if (outname) {
            strncpy(outpath, outname, sizeof(outpath) - 1);
            outpath[sizeof(outpath) - 1] = '\0';
        } else {
            const char *base = strrchr(filename, '/');
            base = base ? base + 1 : filename;
            strncpy(outpath, base, sizeof(outpath) - 1);
            outpath[sizeof(outpath) - 1] = '\0';
            size_t inlen = strlen(outpath);
            if (inlen >= 2 && outpath[inlen - 2] == '.' && outpath[inlen - 1] == 'c')
                outpath[inlen - 1] = 's';
            else
                strncat(outpath, ".s", sizeof(outpath) - inlen - 1);
        }
    }

    parser_t p;
    parser_init(&p, buf, (size_t)size);
    node_t *ast = parser_exec(&p);
    if (debug && ast != NULL) {
        printf("\nParser debug output:\n");
        ast_print(ast, 0);
    }
    if (ast == NULL) {
        parser_finish(&p);
        free(buf);
        return -1;
    }

    if (asm_only) {
        int ret = codegen(ast, outpath);
        node_destroy(ast);
        parser_finish(&p);
        free(buf);
        return ret;
    }

    /* Binary mode: codegen to a temp .s, assemble with as, link with ld */
    char tmpbase[] = "/tmp/guscc_XXXXXX";
    int fd = mkstemp(tmpbase);
    if (fd < 0) {
        perror("mkstemp");
        node_destroy(ast);
        parser_finish(&p);
        free(buf);
        return -1;
    }
    close(fd);
    unlink(tmpbase);

    char asmfile[80], objfile[80];
    snprintf(asmfile, sizeof(asmfile), "%s.s", tmpbase);
    snprintf(objfile, sizeof(objfile), "%s.o", tmpbase);

    int ret = codegen(ast, asmfile);
    node_destroy(ast);
    parser_finish(&p);
    free(buf);
    if (ret != 0) {
        unlink(asmfile);
        return ret;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "as -o %s %s", objfile, asmfile);
    ret = system(cmd);
    unlink(asmfile);
    if (ret != 0) {
        fprintf(stderr, "error: assembler failed\n");
        unlink(objfile);
        return -1;
    }

    const char *binary = outname ? outname : "a.out";
    snprintf(cmd, sizeof(cmd),
             "ld -o %s "
             "-dynamic-linker /lib64/ld-linux-x86-64.so.2 "
             "/usr/lib/x86_64-linux-gnu/crt1.o "
             "/usr/lib/x86_64-linux-gnu/crti.o "
             "%s "
             "-lc "
             "/usr/lib/x86_64-linux-gnu/crtn.o "
             "-L/usr/lib/x86_64-linux-gnu",
             binary, objfile);
    ret = system(cmd);
    unlink(objfile);
    if (ret != 0) {
        fprintf(stderr, "error: linker failed\n");
        return -1;
    }

    return 0;
}
