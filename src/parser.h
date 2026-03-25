#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lex.h"
#include "sym.h"

typedef struct {
    lex_t l;              // Lexer
    token_t *next_token;  // First lookahead (NULL = not loaded)
    token_t *next_token2; // Second lookahead (NULL = not loaded)
    scope_t *scope;       // Current local scope; NULL outside a function
    scope_t *func_scope;  // Flat global scope for function names
    int frame_offset;     // Running stack offset (0 at function entry, decrements)
} parser_t;

void parser_init(parser_t *p, char *buf, size_t size);
void parser_finish(parser_t *p);
node_t *parser_exec(parser_t *p);

#endif /* PARSER_H */
