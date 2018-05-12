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

static int check_next_token(lex_t *l, token_type_t type, const char *sval)
{
    token_t *t = lex_next_token(l);
    ASSERT(type == t->type);
    if (sval) {
        ASSERT(strcmp(sval, t->sval) == 0);
    }
    return 0;
}

static int lex_test_basic(void)
{
    lex_t lex;
    char *src = "int main(void)\n{\n\treturn 42;\n}\n";
    FILE *stream = fmemopen(src, strlen(src), "w+");
    ASSERT(stream != NULL);
    ASSERT(lex_init(&lex, stream) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_INT, NULL) == 0);
    ASSERT(check_next_token(&lex, TOKEN_IDENT, "main") == 0);
    ASSERT(check_next_token(&lex, TOKEN_OPEN_PAR, NULL) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_VOID, NULL) == 0);
    ASSERT(check_next_token(&lex, TOKEN_CLOSE_PAR, NULL) == 0);
    ASSERT(check_next_token(&lex, TOKEN_OPEN_BRACE, NULL) == 0);
    ASSERT(check_next_token(&lex, TOKEN_KW_RETURN, NULL) == 0);
    ASSERT(check_next_token(&lex, TOKEN_NUMBER, "42") == 0);
    ASSERT(check_next_token(&lex, TOKEN_SEMICOLON, NULL) == 0);
    ASSERT(check_next_token(&lex, TOKEN_CLOSE_BRACE, NULL) == 0);
    ASSERT(lex_next_token(&lex) == NULL);
    fclose(stream);
    return 0;
}

void lex_test(void)
{
    ut_run(lex_test_basic);
}
