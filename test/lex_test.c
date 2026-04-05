#include <stdio.h>
#include <string.h>
#include "lex.h"
#include "ut.h"

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
    ASSERT(check_next_token(&lex, '<') == 0);
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
    ASSERT(check_next_token(&lex, '-') == 0);
    ASSERT(check_next_token(&lex, TOKEN_NUM) == 0);
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

static int lex_test_4(void)
{
    lex_t lex;
    char src[] = "a++ --b c->d e<<f g>>h i<=j k>=l m==n o!=p q&&r s||t";
    lex_init(&lex, src, sizeof(src));
    // a++ --b
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "a") == 0);
    ASSERT(check_next_token(&lex, TOKEN_INC_OP) == 0);
    ASSERT(check_next_token(&lex, TOKEN_DEC_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "b") == 0);
    // c->d
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "c") == 0);
    ASSERT(check_next_token(&lex, TOKEN_PTR_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "d") == 0);
    // e<<f
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "e") == 0);
    ASSERT(check_next_token(&lex, TOKEN_LEFT_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "f") == 0);
    // g>>h
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "g") == 0);
    ASSERT(check_next_token(&lex, TOKEN_RIGHT_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "h") == 0);
    // i<=j
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "i") == 0);
    ASSERT(check_next_token(&lex, TOKEN_LE_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "j") == 0);
    // k>=l
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "k") == 0);
    ASSERT(check_next_token(&lex, TOKEN_GE_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "l") == 0);
    // m==n
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "m") == 0);
    ASSERT(check_next_token(&lex, TOKEN_EQ_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "n") == 0);
    // o!=p
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "o") == 0);
    ASSERT(check_next_token(&lex, TOKEN_NE_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "p") == 0);
    // q&&r
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "q") == 0);
    ASSERT(check_next_token(&lex, TOKEN_AND_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "r") == 0);
    // s||t
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "s") == 0);
    ASSERT(check_next_token(&lex, TOKEN_OR_OP) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "t") == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

static int lex_test_5(void)
{
    lex_t lex;
    char src[] = "a*=b b/=c c%=d d+=e e-=f f<<=g g>>=h h&=i i^=j j|=k";
    lex_init(&lex, src, sizeof(src));
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "a") == 0);
    ASSERT(check_next_token(&lex, TOKEN_MUL_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "b") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "b") == 0);
    ASSERT(check_next_token(&lex, TOKEN_DIV_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "c") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "c") == 0);
    ASSERT(check_next_token(&lex, TOKEN_MOD_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "d") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "d") == 0);
    ASSERT(check_next_token(&lex, TOKEN_ADD_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "e") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "e") == 0);
    ASSERT(check_next_token(&lex, TOKEN_SUB_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "f") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "f") == 0);
    ASSERT(check_next_token(&lex, TOKEN_LEFT_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "g") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "g") == 0);
    ASSERT(check_next_token(&lex, TOKEN_RIGHT_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "h") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "h") == 0);
    ASSERT(check_next_token(&lex, TOKEN_AND_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "i") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "i") == 0);
    ASSERT(check_next_token(&lex, TOKEN_XOR_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "j") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "j") == 0);
    ASSERT(check_next_token(&lex, TOKEN_OR_ASSIGN) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "k") == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

static int lex_test_6(void)
{
    lex_t lex;
    char src[] = "aa // line\nbb /* block */ cc sizeof dd ~ ee ? ff : gg . hh";
    lex_init(&lex, src, sizeof(src));
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "aa") == 0);
    // line comment skipped
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "bb") == 0);
    // block comment skipped
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "cc") == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_SIZEOF) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "dd") == 0);
    ASSERT(check_next_token(&lex, '~') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "ee") == 0);
    ASSERT(check_next_token(&lex, '?') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "ff") == 0);
    ASSERT(check_next_token(&lex, ':') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "gg") == 0);
    ASSERT(check_next_token(&lex, '.') == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "hh") == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

static int lex_test_7(void)
{
    lex_t lex;
    char src[] = "c i r v w s ch sizeofx char if int return void while sizeof";
    lex_init(&lex, src, sizeof(src));
    // single-char names that share prefixes with keywords must remain IDENT
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "c") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "i") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "r") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "v") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "w") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "s") == 0);
    // partial / extended keyword names must also remain IDENT
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "ch") == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "sizeofx") == 0);
    // exact keyword matches
    ASSERT(check_next_token(&lex, TOKEN_KW_CHAR) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_IF) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_INT) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_VOID) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_WHILE) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_SIZEOF) == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

static int lex_test_8(void)
{
    lex_t lex;
    char src[] = "typedef typedefx";
    lex_init(&lex, src, sizeof(src));
    ASSERT(check_next_token(&lex, TOKEN_KW_TYPEDEF) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "typedefx") == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

static int lex_test_9(void)
{
    lex_t lex;
    char src[] = "union unionx";
    lex_init(&lex, src, sizeof(src));
    ASSERT(check_next_token(&lex, TOKEN_KW_UNION) == 0);
    ASSERT(check_next_token_str(&lex, TOKEN_IDENT, "unionx") == 0);
    ASSERT(lex_next(&lex) == NULL);
    return 0;
}

void lex_test(void)
{
    ut_run(lex_test_1);
    ut_run(lex_test_2);
    ut_run(lex_test_3);
    ut_run(lex_test_4);
    ut_run(lex_test_5);
    ut_run(lex_test_6);
    ut_run(lex_test_7);
    ut_run(lex_test_8);
    ut_run(lex_test_9);
}
