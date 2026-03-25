#include "sym.h"

#include <stdlib.h>
#include <string.h>

scope_t *scope_new(scope_t *parent)
{
    scope_t *s = malloc(sizeof(scope_t));
    s->syms = NULL;
    s->parent = parent;
    return s;
}

void scope_free(scope_t *scope)
{
    free(scope);
}

void sym_destroy_list(sym_t *sym)
{
    while (sym != NULL) {
        sym_t *next = sym->next;
        free(sym);
        sym = next;
    }
}

sym_t *scope_define(scope_t *scope, const char *name, int name_len, node_t *decl_spec,
                    int pointer_level, int offset)
{
    sym_t *s = malloc(sizeof(sym_t));
    s->name = name;
    s->name_len = name_len;
    s->decl_spec = decl_spec;
    s->pointer_level = pointer_level;
    s->offset = offset;
    s->next = scope->syms;
    scope->syms = s;
    return s;
}

sym_t *scope_lookup(scope_t *scope, const char *name, int name_len)
{
    for (scope_t *sc = scope; sc != NULL; sc = sc->parent) {
        for (sym_t *s = sc->syms; s != NULL; s = s->next) {
            if (s->name_len == name_len && memcmp(s->name, name, (size_t)name_len) == 0)
                return s;
        }
    }
    return NULL;
}
