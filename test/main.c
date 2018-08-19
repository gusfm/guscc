#include "ut.h"

struct ut unit_test;

extern void lex_test(void);
extern void parser_test(void);

int main(void)
{
    lex_test();
    parser_test();
    ut_result();
    return 0;
}
