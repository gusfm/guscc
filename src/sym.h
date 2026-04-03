#ifndef SYM_H
#define SYM_H

#include "ast.h"

/* A single symbol (local variable or parameter) in the symbol table */
typedef struct sym {
    const char *name; /* points into source buffer — NOT null-terminated */
    int name_len;
    node_t *decl_spec; /* ND_DECL_SPEC node describing the type */
    int pointer_level;
    int array_size;  /* 0 = not array, positive = element count */
    int offset;      /* negative offset from %rbp (base of array for arrays) */
    int is_global;   /* 0 = local/param, 1 = global variable */
    struct sym *next; /* next symbol in the same scope */
} sym_t;

/* A single member of a struct definition */
typedef struct struct_member {
    const char *name; /* points into source buffer — NOT null-terminated */
    int name_len;
    node_t *decl_spec; /* ND_DECL_SPEC for the member's type */
    int pointer_level;
    int offset; /* byte offset within the struct */
    int size;   /* byte size of this member */
    struct struct_member *next;
} struct_member_t;

/* A completed struct definition */
typedef struct struct_def {
    const char *tag; /* struct tag name, points into source buffer */
    int tag_len;
    struct_member_t *members; /* linked list of members */
    int size;                 /* total size of the struct (with tail padding) */
    int align;                /* alignment requirement of the struct */
    struct struct_def *next;  /* next definition in the registry */
} struct_def_t;

/* A single lexical scope (function body, nested block, etc.) */
typedef struct scope {
    sym_t *syms;          /* head of symbol list (most-recently-defined first) */
    struct scope *parent; /* enclosing scope; NULL for function-top scope */
} scope_t;

/* Create a new scope with the given parent (NULL for top-level) */
scope_t *scope_new(scope_t *parent);

/* Free the scope struct only. Does NOT free sym_t nodes. */
void scope_free(scope_t *scope);

/* Free a linked list of sym_t nodes (call on scope->syms when done). */
void sym_destroy_list(sym_t *sym);

/* Define a new symbol in scope. Prepends to scope->syms. Returns the entry. */
sym_t *scope_define(scope_t *scope, const char *name, int name_len, node_t *decl_spec,
                    int pointer_level, int array_size, int offset);

/* Look up name walking the parent scope chain. Returns NULL if not found. */
sym_t *scope_lookup(scope_t *scope, const char *name, int name_len);

/* Look up a struct definition by tag name. Returns NULL if not found. */
struct_def_t *struct_def_lookup(struct_def_t *list, const char *tag, int tag_len);

/* Look up a member within a struct definition. Returns NULL if not found. */
struct_member_t *struct_member_lookup(struct_def_t *def, const char *name, int name_len);

/* Free a linked list of struct_member_t nodes. */
void struct_member_destroy_list(struct_member_t *m);

/* Free a linked list of struct_def_t nodes (including their members). */
void struct_def_destroy_list(struct_def_t *d);

#endif /* SYM_H */
