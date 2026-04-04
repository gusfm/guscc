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
    scope_t *func_scope;      // Flat global scope for function names
    scope_t *global_scope;    // Global variable symbol scope
    int frame_offset;         // Running stack offset (0 at function entry, decrements)
    struct_def_t *struct_defs; // Registry of parsed struct definitions
    enum_def_t *enum_defs;    // Registry of parsed enum definitions
    sym_t *enum_syms;         // Enum constant sym_t nodes (owned here, freed at finish)
    int static_local_count;   // Counter for generating unique static local labels
} parser_t;

void parser_init(parser_t *p, char *buf, size_t size);
void parser_finish(parser_t *p);
node_t *parser_exec(parser_t *p);

#endif /* PARSER_H */
