#include "lex.h"
#include "ut.h"

static int basic(void)
{
    lex_t lex;
    ASSERT(lex_init(&lex, "test.c") == 0);
    lex_finish(&lex);
    return 0;
}

void test_lex(void)
{
    ut_run(basic);
}
