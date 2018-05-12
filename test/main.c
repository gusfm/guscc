#include "ut.h"

struct ut unit_test;

extern void lex_test(void);

int main(void)
{
    lex_test();
    ut_result();
    return 0;
}
