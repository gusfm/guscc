#include "lex.h"

int main(void)
{
    lex_t lex;
    if (lex_init(&lex, "test.c") != 0) {
        return -1;
    }
    lex_execute(&lex);
    lex_finish(&lex);
    return 0;
}
