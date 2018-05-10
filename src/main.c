#include "lex.h"

int main(void)
{
    lex_t lex;
    lex_init(&lex, "test.c");
    lex_finish(&lex);
    return 0;
}
