#ifndef LEX_H
#define LEX_H

#include <stdio.h>

typedef struct {
    const char *filename;
    FILE *input;
    int line;
    int col;
    int tok_line;
    int tok_col;
} lex_t;

int lex_init(lex_t *l, const char *filename);
void lex_finish(lex_t *l);
void lex_execute(lex_t *l);

#endif /* LEX_H */
