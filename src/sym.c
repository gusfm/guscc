#include "sym.h"

#include <stdlib.h>
#include <string.h>

#include "ast.h"

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
        free(sym->asm_label);
        free(sym);
        sym = next;
    }
}

sym_t *scope_define(scope_t *scope, const char *name, int name_len, node_t *decl_spec,
                    int pointer_level, int array_size, int offset)
{
    sym_t *s = malloc(sizeof(sym_t));
    s->name = name;
    s->name_len = name_len;
    s->decl_spec = decl_spec;
    s->pointer_level = pointer_level;
    s->array_size = array_size;
    s->offset = offset;
    s->is_global = 0;
    s->is_static = 0;
    s->is_extern = 0;
    s->asm_label = NULL;
    s->asm_label_len = 0;
    s->is_enum_const = 0;
    s->enum_val = 0;
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

struct_def_t *struct_def_lookup(struct_def_t *list, const char *tag, int tag_len)
{
    for (struct_def_t *d = list; d != NULL; d = d->next) {
        if (d->tag_len == tag_len && memcmp(d->tag, tag, (size_t)tag_len) == 0)
            return d;
    }
    return NULL;
}

struct_member_t *struct_member_lookup(struct_def_t *def, const char *name, int name_len)
{
    for (struct_member_t *m = def->members; m != NULL; m = m->next) {
        if (m->name_len == name_len && memcmp(m->name, name, (size_t)name_len) == 0)
            return m;
    }
    return NULL;
}

void struct_member_destroy_list(struct_member_t *m)
{
    while (m != NULL) {
        struct_member_t *next = m->next;
        node_destroy(m->decl_spec);
        free(m);
        m = next;
    }
}

void struct_def_destroy_list(struct_def_t *d)
{
    while (d != NULL) {
        struct_def_t *next = d->next;
        struct_member_destroy_list(d->members);
        free(d);
        d = next;
    }
}

enum_def_t *enum_def_lookup(enum_def_t *list, const char *tag, int tag_len)
{
    for (enum_def_t *d = list; d != NULL; d = d->next) {
        if (d->tag_len == tag_len && memcmp(d->tag, tag, (size_t)tag_len) == 0)
            return d;
    }
    return NULL;
}

void enum_def_destroy_list(enum_def_t *d)
{
    while (d != NULL) {
        enum_def_t *next = d->next;
        free(d);
        d = next;
    }
}
