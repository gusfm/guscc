#ifndef PARSER_H
#define PARSER_H

#include "lex.h"

typedef struct {
    lex_t l;
    token_t *saved_tok;
} parser_t;

int parser_init(parser_t *p, FILE *input);
void parser_next(parser_t *p);

#endif /* PARSER_H */
