#include "lex.h"

int main(void)
{
    lex_t lex;
    const char *filename = "test.c";
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "error: could not open %s\n", filename);
        return -1;
    }
    if (lex_init(&lex, f) != 0) {
        return -1;
    }
    fclose(f);
    return 0;
}
