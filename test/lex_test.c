#include <stdio.h>
#include <string.h>
#include "lex.h"
#include "ut.h"

#if defined(__STRICT_ANSI__)

static FILE *fmemopen(void *buf, size_t size, const char *mode)
{
    size_t written;
    FILE *f = fopen("fmemopen_tmp.c", mode);
    if (f == NULL) {
        perror("fopen");
        return NULL;
    }
    written = fwrite(buf, size, 1, f);
    if (written != 1) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    return f;
}
#endif

static int check_next_token(lex_t *l, token_type_t type)
{
    token_t *t = lex_next_token(l);
    ASSERT(type == t->type);
    token_destroy(t);
    return 0;
}

static int check_next_token_char(lex_t *l, token_type_t type, char c)
{
    token_t *t = lex_next_token(l);
    ASSERT(type == t->type);
    ASSERT(c == t->val.c);
    token_destroy(t);
    return 0;
}

static int check_next_token_str(lex_t *l, token_type_t type, const char *s)
{
    token_t *t = lex_next_token(l);
    ASSERT(type == t->type);
    ASSERT(strcmp(s, t->val.s) == 0);
    token_destroy(t);
    return 0;
}

static int lex_test_1(void)
{
    lex_t lex;
    char *src = "int main(void)\n{\n\treturn 42;\n}\n";
    FILE *stream = fmemopen(src, strlen(src), "w+");
    ASSERT(stream != NULL);
    ASSERT(lex_init(&lex, stream) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "main") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_VOID) == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, '{') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "42") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, '}') == 0);
    ASSERT(lex_next_token(&lex) == NULL);
    fclose(stream);
    return 0;
}

static int lex_test_2(void)
{
    lex_t lex;
    char *src = "int main(void){if(1 < 2)return 0;else if (3 <= 4) return 1; "
                "else 1 << 2; x<<=1; return 0;}";
    FILE *stream = fmemopen(src, strlen(src), "w+");
    ASSERT(stream != NULL);
    ASSERT(lex_init(&lex, stream) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "main") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_VOID) == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, '{') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_IF) == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "1") == 0);
    ASSERT(check_next_token(&lex, '<') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "2") == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "0") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_ELSE) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_IF) == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "3") == 0);
    ASSERT(check_next_token(&lex, TOKEN_LE_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "4") == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "1") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_ELSE) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "1") == 0);
    ASSERT(check_next_token(&lex, TOKEN_LSH_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "2") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "x") == 0);
    ASSERT(check_next_token(&lex, TOKEN_LSH_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "1") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "0") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, '}') == 0);
    ASSERT(lex_next_token(&lex) == NULL);
    fclose(stream);
    return 0;
}

static int lex_test_3(void)
{
    lex_t lex;
    char *src =
        "int a = 0; printf(\"Hello World %d %s\n\", a, \"a\"); a += b; a-= b;";
    FILE *stream = fmemopen(src, strlen(src), "w+");
    ASSERT(stream != NULL);
    ASSERT(lex_init(&lex, stream) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "a") == 0);
    ASSERT(check_next_token(&lex, '=') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_NUMBER, "0") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "printf") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_STRING_LITERAL,
                                "Hello World %d %s\n") == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "a") == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_STRING_LITERAL, "a") == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "a") == 0);
    ASSERT(check_next_token(&lex, TOKEN_ADD_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "b") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "a") == 0);
    ASSERT(check_next_token(&lex, TOKEN_SUB_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "b") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(lex_next_token(&lex) == NULL);
    fclose(stream);
    return 0;
}

static int lex_test_4(void)
{
    lex_t lex;
    char *src = "typedef struct {char a; char *b; size_t c;} xyz_t; xyz_t x; "
                "x.a = 'a'; x.b = &string; x.c = sizeof(x);";
    FILE *stream = fmemopen(src, strlen(src), "w+");
    ASSERT(stream != NULL);
    ASSERT(lex_init(&lex, stream) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_TYPEDEF) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_STRUCT) == 0);
    ASSERT(check_next_token(&lex, '{') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_CHAR) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "a") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_CHAR) == 0);
    ASSERT(check_next_token(&lex, '*') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "b") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "size_t") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "c") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, '}') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "xyz_t") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "xyz_t") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "x") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "x") == 0);
    ASSERT(check_next_token(&lex, '.') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "a") == 0);
    ASSERT(check_next_token(&lex, '=') == 0);
    ASSERT(check_next_token_char(&lex, TOKEN_CHAR, 'a') == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "x") == 0);
    ASSERT(check_next_token(&lex, '.') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "b") == 0);
    ASSERT(check_next_token(&lex, '=') == 0);
    ASSERT(check_next_token(&lex, '&') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "string") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "x") == 0);
    ASSERT(check_next_token(&lex, '.') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "c") == 0);
    ASSERT(check_next_token(&lex, '=') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_SIZEOF) == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "x") == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(lex_next_token(&lex) == NULL);
    fclose(stream);
    return 0;
}

void lex_test(void)
{
    ut_run(lex_test_1);
    ut_run(lex_test_2);
    ut_run(lex_test_3);
    ut_run(lex_test_4);
}
