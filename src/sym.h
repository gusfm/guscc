#ifndef SYM_H
#define SYM_H

#include "ast.h"

/* A single symbol (local variable or parameter) in the symbol table */
typedef struct sym {
    const char *name; /* points into source buffer — NOT null-terminated */
    int name_len;
    node_t *decl_spec; /* ND_DECL_SPEC node describing the type */
    int pointer_level;
    int offset;       /* negative offset from %rbp, e.g. -4, -8 */
    struct sym *next; /* next symbol in the same scope */
} sym_t;

/* A single lexical scope (function body, nested block, etc.) */
typedef struct scope {
    sym_t *syms; /* head of symbol list (most-recently-defined first) */
    struct scope *parent; /* enclosing scope; NULL for function-top scope */
} scope_t;

/* Create a new scope with the given parent (NULL for top-level) */
scope_t *scope_new(scope_t *parent);

/* Free the scope struct only. Does NOT free sym_t nodes. */
void scope_free(scope_t *scope);

/* Free a linked list of sym_t nodes (call on scope->syms when done). */
void sym_destroy_list(sym_t *sym);

/* Define a new symbol in scope. Prepends to scope->syms. Returns the entry. */
sym_t *scope_define(scope_t *scope, const char *name, int name_len,
                    node_t *decl_spec, int pointer_level, int offset);

/* Look up name walking the parent scope chain. Returns NULL if not found. */
sym_t *scope_lookup(scope_t *scope, const char *name, int name_len);

#endif /* SYM_H */
