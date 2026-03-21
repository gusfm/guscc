#ifndef PARSER_H
#define PARSER_H

#include "lex.h"
#include "ast.h"

typedef struct {
    lex_t l;             // Lexer
    token_t *next_token; // Cached lookahead token (NULL = not loaded)
} parser_t;

void parser_init(parser_t *p, char *buf, size_t size);
void parser_finish(parser_t *p);
node_t *parser_exec(parser_t *p);

#endif /* PARSER_H */
