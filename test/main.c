#include "ut.h"

struct ut unit_test;

extern void test_lex(void);

int main(void)
{
    test_lex();
    ut_result();
    return 0;
}
