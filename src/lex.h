#ifndef LEX_H
#define LEX_H

#include <stdio.h>
#include "token.h"

typedef struct {
    FILE *input;
    int line;
    int col;
} lex_t;

int lex_init(lex_t *l, FILE *input);
token_t *lex_next_token(lex_t *l);

#endif /* LEX_H */
