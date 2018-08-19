#include "parser.h"

int main(void)
{
    parser_t parser;
    const char *filename = "test.c";
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "error: could not open %s\n", filename);
        return -1;
    }
    parser_init(&parser, f);
    parser_next(&parser);
    fclose(f);
    return 0;
}
