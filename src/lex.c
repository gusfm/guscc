#include "lex.h"

int lex_init(lex_t *lex, const char *filename)
{
    lex->filename = filename;
    lex->input = fopen(filename, "r");
    if (lex->input == NULL) {
        fprintf(stderr, "error: could not open %s\n", filename);
        return -1;
    }
    return 0;
}

void lex_finish(lex_t *lex)
{
    fclose(lex->input);
}
