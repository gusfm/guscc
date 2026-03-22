#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lex.h"

typedef struct {
    lex_t l;              // Lexer
    token_t *next_token;  // First lookahead (NULL = not loaded)
    token_t *next_token2; // Second lookahead (NULL = not loaded)
} parser_t;

void parser_init(parser_t *p, char *buf, size_t size);
void parser_finish(parser_t *p);
node_t *parser_exec(parser_t *p);

#endif /* PARSER_H */
