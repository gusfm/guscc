#include <stdio.h>
#include <string.h>
#include "lex.h"
#include "ut.h"
#include "utils.h"

static int check_next_token(lex_t *l, token_type_t type)
{
    token_t *t = lex_next(l);
    ASSERT(type == t->type);
    token_destroy(t);
    return 0;
}

static int check_next_token_str(lex_t *l, token_type_t type, const char *s)
{
    token_t *t = lex_next(l);
    ASSERT(type == t->type);
    ASSERT(strncmp(s, t->sval, t->len) == 0);
    token_destroy(t);
    return 0;
}

static int lex_test_1(void)
{
    lex_t lex;
    char src[] = "int main(void) { return 42; }";
    lex_init(&lex, src, sizeof(src));
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "main") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_VOID) == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, '{') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token(&lex, TOKEN_NUM) == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, '}') == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

static int lex_test_2(void)
{
    lex_t lex;
    char src[] = "int main(int argc, char **argv) { return 42; }";
    lex_init(&lex, src, sizeof(src));
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "main") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "argc") == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_CHAR) == 0);
    ASSERT(check_next_token(&lex, '*') == 0);
    ASSERT(check_next_token(&lex, '*') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "argv") == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, '{') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token(&lex, TOKEN_NUM) == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, '}') == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

static int lex_test_3(void)
{
    lex_t lex;
    char src[] =
        "void func(char *str, int val) { printf(\"%s:%d\n\", str, val); }"
        "int main(int argc, char **argv) { if (argc < 2) { fprintf(stderr, "
        "\"test\n\"); return -1; } func(argv[1], 2); return 0;}";
    lex_init(&lex, src, sizeof(src));
    // void func(char *str, int val) {
    ASSERT(check_next_token(&lex, TOKEN_KW_VOID) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "func") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_CHAR) == 0);
    ASSERT(check_next_token(&lex, '*') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "str") == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "val") == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, '{') == 0);
    // printf(...)}
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "printf") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token(&lex, TOKEN_STR) == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "str") == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "val") == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, '}') == 0);
    // int main(int argc, char **argv) {
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "main") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "argc") == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_CHAR) == 0);
    ASSERT(check_next_token(&lex, '*') == 0);
    ASSERT(check_next_token(&lex, '*') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "argv") == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, '{') == 0);
    // if (argc < 2)
    ASSERT(check_next_token(&lex, TOKEN_KW_IF) == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "argc") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "<") == 0);
    ASSERT(check_next_token(&lex, TOKEN_NUM) == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, '{') == 0);
    // fprintf(stderr, "test\n");
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "fprintf") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "stderr") == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token(&lex, TOKEN_STR) == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    // return -1;
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "-1") == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, '}') == 0);
    // func(argv[1], 2);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "func") == 0);
    ASSERT(check_next_token(&lex, '(') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "argv") == 0);
    ASSERT(check_next_token(&lex, '[') == 0);
    ASSERT(check_next_token(&lex, TOKEN_NUM) == 0);
    ASSERT(check_next_token(&lex, ']') == 0);
    ASSERT(check_next_token(&lex, ',') == 0);
    ASSERT(check_next_token(&lex, TOKEN_NUM) == 0);
    ASSERT(check_next_token(&lex, ')') == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    // return 0; }
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token(&lex, TOKEN_NUM) == 0);
    ASSERT(check_next_token(&lex, ';') == 0);
    ASSERT(check_next_token(&lex, '}') == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

void lex_test(void)
{
    ut_run(lex_test_1);
    ut_run(lex_test_2);
    ut_run(lex_test_3);
}
