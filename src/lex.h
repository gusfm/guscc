#ifndef LEX_H
#define LEX_H

#include <stdio.h>

typedef struct {
    const char *filename;
    FILE *input;
} lex_t;

int lex_init(lex_t *lex, const char *filename);
void lex_finish(lex_t *lex);

#endif /* LEX_H */
