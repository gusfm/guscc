#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "sym.h"

// Declarator mode: controls whether an identifier is required, forbidden, or optional.
typedef enum {
    DECL_CONCRETE, // identifier required (function defs, local decls, struct members)
    DECL_ABSTRACT, // identifier forbidden (type_name: casts, sizeof)
    DECL_EITHER,   // identifier optional (parameter declarations)
} decl_mode_t;

// Forward declarations
static node_t *parser_declarator(parser_t *p);
static node_t *parser_declarator_mode(parser_t *p, decl_mode_t mode);
static node_t *parser_compound_statement(parser_t *p);
static node_t *parser_statement(parser_t *p);
static node_t *parser_declaration_specifiers(parser_t *p);
static node_t *parser_expression(parser_t *p);
static node_t *parser_assignment_expression(parser_t *p);
static node_t *parser_cast_expression(parser_t *p);
static node_t *parser_unary_expression(parser_t *p);
static int parser_sym_size(node_t *decl_spec, int pointer_level);

void parser_init(parser_t *p, char *buf, size_t size)
{
    lex_init(&p->l, buf, size);
    p->next_token = NULL;
    p->next_token2 = NULL;
    p->scope = NULL;
    p->func_scope = scope_new(NULL);
    p->global_scope = scope_new(NULL);
    p->frame_offset = 0;
    p->struct_defs = NULL;
    p->enum_defs = NULL;
    p->enum_syms = NULL;
    p->typedef_defs = NULL;
    p->typedef_pointer_level = 0;
    p->static_local_count = 0;
}

void parser_finish(parser_t *p)
{
    if (p->next_token) {
        token_destroy(p->next_token);
        p->next_token = NULL;
    }
    if (p->next_token2) {
        token_destroy(p->next_token2);
        p->next_token2 = NULL;
    }
    if (p->func_scope) {
        sym_destroy_list(p->func_scope->syms);
        scope_free(p->func_scope);
        p->func_scope = NULL;
    }
    if (p->global_scope) {
        sym_destroy_list(p->global_scope->syms);
        scope_free(p->global_scope);
        p->global_scope = NULL;
    }
    struct_def_destroy_list(p->struct_defs);
    p->struct_defs = NULL;
    enum_def_destroy_list(p->enum_defs);
    p->enum_defs = NULL;
    sym_destroy_list(p->enum_syms);
    p->enum_syms = NULL;
    typedef_def_destroy_list(p->typedef_defs);
    p->typedef_defs = NULL;
}

token_t *parser_peek(parser_t *p)
{
    if (p->next_token == NULL) {
        p->next_token = lex_next(&p->l);
    }
    return p->next_token;
}

token_t *parser_next(parser_t *p)
{
    if (p->next_token != NULL) {
        token_t *t = p->next_token;
        p->next_token = p->next_token2;
        p->next_token2 = NULL;
        return t;
    }
    return lex_next(&p->l);
}

static token_t *parser_peek2(parser_t *p)
{
    parser_peek(p); // ensure next_token is loaded
    if (p->next_token2 == NULL)
        p->next_token2 = lex_next(&p->l);
    return p->next_token2;
}

bool parser_accept(parser_t *p, token_type_t type)
{
    token_t *t = parser_peek(p);
    if (t->type == type) {
        token_destroy(parser_next(p)); // consume and free
        return true;
    }
    return false;
}

token_t *parser_expect_token(parser_t *p, token_type_t type)
{
    token_t *t = parser_next(p);
    if (t->type == type) {
        return t;
    } else {
        char str[100];
        fprintf(stderr, "Expected %s but received '%.*s'\n", token_type_to_str(type, str, 100),
                t->len, t->sval);
        token_destroy(t);
        return NULL;
    }
}

bool parser_expect(parser_t *p, token_type_t type)
{
    token_t *t = parser_expect_token(p, type);
    if (t != NULL) {
        token_destroy(t);
        return true;
    }
    return false;
}

static int parser_align_up(int offset, int align)
{
    return (offset + align - 1) & ~(align - 1);
}

/*
 * struct_or_union_specifier
 *  : struct_or_union IDENTIFIER '{' struct_declaration_list '}'
 *  | struct_or_union '{' struct_declaration_list '}'
 *  | struct_or_union IDENTIFIER
 *  ;
 * struct_or_union
 *  : STRUCT
 *  | UNION
 *  ;
 */
static node_t *parser_struct_or_union_specifier(parser_t *p)
{
    token_t *kw = parser_peek(p);
    int is_union = 0;
    if (kw != NULL && kw->type == TOKEN_KW_UNION)
        is_union = 1;
    else if (kw == NULL || kw->type != TOKEN_KW_STRUCT) {
        fprintf(stderr, "Expected 'struct' or 'union'\n");
        return NULL;
    }
    kw = parser_next(p);
    int line = kw->line, col = kw->col;
    token_destroy(kw);

    // Optional tag name
    char *tag = NULL;
    int tag_len = 0;
    token_t *peek = parser_peek(p);
    if (peek != NULL && peek->type == TOKEN_IDENT) {
        token_t *ident = parser_next(p);
        tag = ident->sval;
        tag_len = ident->len;
        token_destroy(ident);
    }

    // If '{' follows, parse struct/union body
    if (parser_accept(p, '{')) {
        struct_member_t *members = NULL;
        struct_member_t **tail = &members;
        int offset = 0;
        int max_align = 1;
        int max_size = 0; /* for unions: track largest member */

        while (!parser_accept(p, '}')) {
            node_t *mem_decl_spec = parser_declaration_specifiers(p);
            if (mem_decl_spec == NULL) {
                struct_member_destroy_list(members);
                return NULL;
            }
            node_t *mem_declarator = parser_declarator(p);
            if (mem_declarator == NULL) {
                node_destroy(mem_decl_spec);
                struct_member_destroy_list(members);
                return NULL;
            }
            if (!parser_expect(p, ';')) {
                node_destroy(mem_decl_spec);
                node_destroy(mem_declarator);
                struct_member_destroy_list(members);
                return NULL;
            }

            int ptr_lvl = mem_declarator->direct_decl.pointer_level;
            int mem_size = parser_sym_size(mem_decl_spec, ptr_lvl);
            int mem_align = mem_size < 8 ? mem_size : 8;

            if (mem_align > max_align)
                max_align = mem_align;

            struct_member_t *m = malloc(sizeof(struct_member_t));
            m->name = mem_declarator->direct_decl.ident.str;
            m->name_len = mem_declarator->direct_decl.ident.len;
            m->decl_spec = mem_decl_spec;
            m->pointer_level = ptr_lvl;
            m->size = mem_size;
            m->next = NULL;
            *tail = m;
            tail = &m->next;

            if (is_union) {
                m->offset = 0;
                if (mem_size > max_size)
                    max_size = mem_size;
            } else {
                offset = parser_align_up(offset, mem_align);
                m->offset = offset;
                offset += mem_size;
            }
            node_destroy(mem_declarator);
        }

        // Total size with tail padding
        int total_size = is_union ? parser_align_up(max_size, max_align)
                                  : parser_align_up(offset, max_align);

        struct_def_t *def = malloc(sizeof(struct_def_t));
        def->tag = tag;
        def->tag_len = tag_len;
        def->members = members;
        def->size = total_size;
        def->align = max_align;
        def->is_union = is_union;
        def->next = p->struct_defs;
        p->struct_defs = def;

        node_t *node = node_create(ND_STRUCT_SPEC, line, col);
        node->struct_spec.tag.str = tag;
        node->struct_spec.tag.len = tag_len;
        node->struct_spec.def = def;
        return node;
    }

    // No body — look up existing definition
    if (tag == NULL) {
        fprintf(stderr, "%d:%d: error: expected %s tag or body\n", line, col,
                is_union ? "union" : "struct");
        return NULL;
    }

    struct_def_t *def = struct_def_lookup(p->struct_defs, tag, tag_len);
    if (def == NULL) {
        fprintf(stderr, "%d:%d: error: use of undefined %s '%.*s'\n", line, col,
                is_union ? "union" : "struct", tag_len, tag);
        return NULL;
    }

    node_t *node = node_create(ND_STRUCT_SPEC, line, col);
    node->struct_spec.tag.str = tag;
    node->struct_spec.tag.len = tag_len;
    node->struct_spec.def = def;
    return node;
}

/*
 * type_specifier
 *  : VOID
 *  | CHAR
 *  | SHORT
 *  | INT
 *  | LONG
 *  | FLOAT
 *  | DOUBLE
 *  | SIGNED
 *  | UNSIGNED
 *  | struct_or_union_specifier
 *  | enum_specifier
 *  | TYPE_NAME
 *  ;
 */
/* Helper: evaluate a constant integer expression from a token stream.
 * Handles integer literals and negated literals only (for enum initializers). */
static int parser_const_int(parser_t *p)
{
    int sign = 1;
    if (parser_peek(p)->type == '-') {
        token_destroy(parser_next(p));
        sign = -1;
    }
    token_t *tok = parser_next(p);
    int val = 0;
    for (int i = 0; i < tok->len; i++)
        val = val * 10 + (tok->sval[i] - '0');
    token_destroy(tok);
    return sign * val;
}

/*
 * enum_specifier
 *  : ENUM '{' enumerator_list '}'
 *  | ENUM IDENTIFIER '{' enumerator_list '}'
 *  | ENUM IDENTIFIER
 *  ;
 * enumerator_list
 *  : enumerator
 *  | enumerator_list ',' enumerator
 *  ;
 * enumerator
 *  : IDENTIFIER
 *  | IDENTIFIER '=' constant_expression
 *  ;
 */
/* Look up an enum constant by name in the parser's enum_syms list. */
static sym_t *parser_enum_lookup(parser_t *p, const char *name, int name_len)
{
    for (sym_t *s = p->enum_syms; s != NULL; s = s->next) {
        if (s->name_len == name_len && memcmp(s->name, name, (size_t)name_len) == 0)
            return s;
    }
    return NULL;
}

static node_t *parser_enum_specifier(parser_t *p)
{
    token_t *kw = parser_next(p); // consume 'enum'
    int line = kw->line, col = kw->col;
    token_destroy(kw);

    // Optional tag name
    char *tag = NULL;
    int tag_len = 0;
    if (parser_peek(p)->type == TOKEN_IDENT) {
        token_t *id = parser_next(p);
        tag = id->sval;
        tag_len = id->len;
        token_destroy(id);
    }

    if (parser_peek(p)->type != '{') {
        // Forward reference: enum NAME (without body)
        if (tag == NULL) {
            fprintf(stderr, "%d:%d: error: expected enum tag or '{'\n", line, col);
            return NULL;
        }
        enum_def_t *def = enum_def_lookup(p->enum_defs, tag, tag_len);
        if (def == NULL) {
            fprintf(stderr, "%d:%d: error: use of undefined enum '%.*s'\n", line, col, tag_len,
                    tag);
            return NULL;
        }
        node_t *n = node_create(ND_ENUM_SPEC, line, col);
        n->enum_spec.tag.str = tag;
        n->enum_spec.tag.len = tag_len;
        return n;
    }

    // Consume '{'
    token_destroy(parser_next(p));

    // Parse enumerator list
    int next_val = 0;
    while (parser_peek(p)->type != '}') {
        token_t *id = parser_next(p);
        if (id->type != TOKEN_IDENT) {
            token_print_error(id, "identifier");
            token_destroy(id);
            return NULL;
        }
        int val = next_val;
        if (parser_peek(p)->type == '=') {
            token_destroy(parser_next(p)); // consume '='
            val = parser_const_int(p);
        }

        // Create enum constant sym and prepend to parser's enum_syms list
        sym_t *sym = malloc(sizeof(sym_t));
        memset(sym, 0, sizeof(sym_t));
        sym->name = id->sval;
        sym->name_len = id->len;
        sym->is_enum_const = 1;
        sym->enum_val = val;
        sym->next = p->enum_syms;
        p->enum_syms = sym;

        next_val = val + 1;
        token_destroy(id);
        if (parser_peek(p)->type == ',')
            token_destroy(parser_next(p));
    }
    token_destroy(parser_next(p)); // consume '}'

    // Register named enum in the registry
    if (tag) {
        enum_def_t *def = malloc(sizeof(enum_def_t));
        def->tag = tag;
        def->tag_len = tag_len;
        def->next = p->enum_defs;
        p->enum_defs = def;
    }

    node_t *n = node_create(ND_ENUM_SPEC, line, col);
    n->enum_spec.tag.str = tag;
    n->enum_spec.tag.len = tag_len;
    return n;
}

static node_t *parser_type_specifier(parser_t *p)
{
    token_t *peek = parser_peek(p);
    if (peek != NULL && (peek->type == TOKEN_KW_STRUCT || peek->type == TOKEN_KW_UNION))
        return parser_struct_or_union_specifier(p);
    if (peek != NULL && peek->type == TOKEN_KW_ENUM)
        return parser_enum_specifier(p);

    // Check for typedef name (TYPE_NAME in the grammar)
    if (peek != NULL && peek->type == TOKEN_IDENT) {
        typedef_def_t *td = typedef_def_lookup(p->typedef_defs, peek->sval, peek->len);
        if (td != NULL) {
            token_t *tok = parser_next(p);
            node_t *orig = td->decl_spec->decl_spec.type_spec;
            node_t *cloned = node_create(orig->kind, tok->line, tok->col);
            if (orig->kind == ND_TYPE_SPEC) {
                cloned->type_spec = orig->type_spec;
            } else if (orig->kind == ND_STRUCT_SPEC) {
                cloned->struct_spec.tag = orig->struct_spec.tag;
                cloned->struct_spec.def = orig->struct_spec.def;
            } else if (orig->kind == ND_ENUM_SPEC) {
                cloned->enum_spec.tag = orig->enum_spec.tag;
            }
            p->typedef_pointer_level = td->pointer_level;
            token_destroy(tok);
            return cloned;
        }
    }

    token_t *tok = parser_next(p);
    node_t *node = node_create(ND_TYPE_SPEC, tok->line, tok->col);
    if (tok->type == TOKEN_KW_VOID) {
        node->type_spec = ND_TYPE_VOID;
    } else if (tok->type == TOKEN_KW_CHAR) {
        node->type_spec = ND_TYPE_CHAR;
    } else if (tok->type == TOKEN_KW_SHORT) {
        node->type_spec = ND_TYPE_SHORT;
        // Consume optional trailing 'int': "short int"
        token_t *next = parser_peek(p);
        if (next && next->type == TOKEN_KW_INT) {
            token_t *consumed = parser_next(p);
            token_destroy(consumed);
        }
    } else if (tok->type == TOKEN_KW_INT) {
        node->type_spec = ND_TYPE_INT;
    } else if (tok->type == TOKEN_KW_LONG) {
        node->type_spec = ND_TYPE_LONG;
        // Consume optional second 'long': "long long"
        token_t *next = parser_peek(p);
        if (next && next->type == TOKEN_KW_LONG) {
            token_t *consumed = parser_next(p);
            token_destroy(consumed);
            next = parser_peek(p);
        }
        // Consume optional trailing 'int': "long int" or "long long int"
        if (next && next->type == TOKEN_KW_INT) {
            token_t *consumed = parser_next(p);
            token_destroy(consumed);
        }
    } else {
        token_print_error(tok, "type");
        node_destroy(node);
        node = NULL;
    }
    token_destroy(tok);
    return node;
}

static node_t *parser_declaration_specifiers(parser_t *p)
{
    int storage_class = SC_NONE;
    int type_qualifier = TQ_NONE;
    // Consume storage class specifiers and type qualifiers before the type specifier
    for (;;) {
        token_t *peek = parser_peek(p);
        if (peek && (peek->type == TOKEN_KW_STATIC || peek->type == TOKEN_KW_EXTERN ||
                     peek->type == TOKEN_KW_TYPEDEF)) {
            if (peek->type == TOKEN_KW_STATIC)
                storage_class = SC_STATIC;
            else if (peek->type == TOKEN_KW_EXTERN)
                storage_class = SC_EXTERN;
            else
                storage_class = SC_TYPEDEF;
            token_destroy(parser_next(p));
        } else if (peek && peek->type == TOKEN_KW_CONST) {
            type_qualifier |= TQ_CONST;
            token_destroy(parser_next(p));
        } else {
            break;
        }
    }
    p->typedef_pointer_level = 0;
    node_t *type_spec = parser_type_specifier(p);
    if (type_spec == NULL)
        return NULL;
    int typedef_ptrlvl = p->typedef_pointer_level;
    // Accept trailing const after type specifier (e.g. "int const x")
    if (parser_peek(p)->type == TOKEN_KW_CONST) {
        type_qualifier |= TQ_CONST;
        token_destroy(parser_next(p));
    }
    node_t *node = node_create(ND_DECL_SPEC, type_spec->line, type_spec->col);
    node->decl_spec.type_spec = type_spec;
    node->decl_spec.pointer_level = typedef_ptrlvl;
    node->decl_spec.storage_class = storage_class;
    node->decl_spec.type_qualifier = type_qualifier;
    return node;
}

static node_t *parser_parameter_declaration(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL)
        return NULL;

    // If next token is ')' or ',' — no declarator (bare type specifier)
    token_t *peek = parser_peek(p);
    if (peek->type == ')' || peek->type == ',') {
        node_t *n = node_create(ND_PARAM_DECL, decl_spec->line, decl_spec->col);
        n->param_decl.decl_spec = decl_spec;
        n->param_decl.declarator = NULL;
        return n;
    }

    // Parse declarator (concrete or abstract)
    node_t *declarator = parser_declarator_mode(p, DECL_EITHER);

    node_t *n = node_create(ND_PARAM_DECL, decl_spec->line, decl_spec->col);
    n->param_decl.decl_spec = decl_spec;
    n->param_decl.declarator = declarator;
    return n;
}

static node_t *parser_parameter_list(parser_t *p)
{
    node_t *param_decl = parser_parameter_declaration(p);
    if (param_decl == NULL)
        return NULL;

    node_t *n = node_create(ND_PARAM_LIST, param_decl->line, param_decl->col);
    n->param_list.params[0] = param_decl;
    n->param_list.is_variadic = 0;

    int nparams = 1;
    while (parser_accept(p, ',')) {
        if (parser_peek(p)->type == TOKEN_ELLIPSIS) {
            token_t *tok = parser_next(p);
            token_destroy(tok);
            n->param_list.is_variadic = 1;
            break;
        }
        param_decl = parser_parameter_declaration(p);
        if (param_decl == NULL)
            return NULL;
        n->param_list.params[nparams++] = param_decl;
    }
    n->param_list.nparams = nparams;
    return n;
}

static node_t *parser_direct_declarator(parser_t *p, decl_mode_t mode)
{
    node_t *n;

    if (parser_peek(p)->type == TOKEN_IDENT && mode != DECL_ABSTRACT) {
        // Concrete path: consume identifier
        token_t *ident = parser_expect_token(p, TOKEN_IDENT);
        n = node_create(ND_DIRECT_DECL, ident->line, ident->col);
        n->direct_decl.ident.str = ident->sval;
        n->direct_decl.ident.len = ident->len;
        token_destroy(ident);
    } else if (mode == DECL_CONCRETE) {
        // Must have identifier — emit error via expect_token
        parser_expect_token(p, TOKEN_IDENT);
        return NULL;
    } else {
        // Abstract or Either with no identifier
        token_t *tok = parser_peek(p);
        n = node_create(ND_DIRECT_DECL, tok->line, tok->col);
        n->direct_decl.ident.str = NULL;
        n->direct_decl.ident.len = 0;
    }
    n->direct_decl.param_list = NULL;
    n->direct_decl.pointer_level = 0;
    n->direct_decl.array_size = 0;

    // Array dimension suffix: '[' constant_expression ']' or '[' ']'
    if (parser_peek(p)->type == '[') {
        token_t *lb = parser_next(p); // consume '['
        token_destroy(lb);
        if (parser_peek(p)->type == ']') {
            token_t *rb = parser_next(p); // consume ']'
            token_destroy(rb);
            n->direct_decl.array_size = -1; // unsized — valid only for params
        } else {
            node_t *size_expr = parser_assignment_expression(p);
            if (size_expr == NULL) {
                node_destroy(n);
                return NULL;
            }
            if (size_expr->kind != ND_NUM) {
                fprintf(stderr, "%d:%d: error: array size must be an integer constant\n",
                        size_expr->line, size_expr->col);
                node_destroy(size_expr);
                node_destroy(n);
                return NULL;
            }
            char tmp[32];
            int len = size_expr->num.val.len < 31 ? size_expr->num.val.len : 31;
            memcpy(tmp, size_expr->num.val.str, (size_t)len);
            tmp[len] = '\0';
            n->direct_decl.array_size = (int)strtol(tmp, NULL, 10);
            node_destroy(size_expr);
            if (!parser_expect(p, ']')) {
                node_destroy(n);
                return NULL;
            }
        }
    }

    // Suffix: optional '(' parameter_list ')' or grouping '(' abstract_declarator ')'
    if (parser_peek(p)->type == '(') {
        token_t *p2 = parser_peek2(p);
        bool is_grouping = (p2 != NULL && p2->type == '*');

        if (is_grouping && mode != DECL_CONCRETE) {
            // Grouping: '(' abstract_declarator ')'
            token_t *lp = parser_next(p); // consume '('
            token_destroy(lp);
            node_t *inner = parser_declarator_mode(p, mode);
            if (!inner || !parser_expect(p, ')')) {
                node_destroy(n);
                node_destroy(inner);
                return NULL;
            }
            // Merge pointer levels from inner declarator
            n->direct_decl.pointer_level += inner->direct_decl.pointer_level;
            node_destroy(inner);
            // After grouping, check for trailing '(' param_list ')' suffix
            if (parser_peek(p)->type != '(')
                return n;
        }

        if (parser_accept(p, '(')) {
            if (parser_accept(p, ')'))
                return n;
            node_t *param_list = parser_parameter_list(p);
            if (!param_list) {
                node_destroy(n);
                return NULL;
            }
            if (!parser_expect(p, ')')) {
                node_destroy(n);
                return NULL;
            }
            n->direct_decl.param_list = param_list;
        }
    }
    return n;
}

static int parser_pointer_ex(parser_t *p, int *trailing_const)
{
    int pointer_level = 0;
    *trailing_const = 0;
    while (parser_accept(p, '*')) {
        pointer_level++;
        *trailing_const = 0;
        // Skip optional type qualifiers after '*' (e.g. "* const")
        while (parser_peek(p)->type == TOKEN_KW_CONST) {
            token_destroy(parser_next(p));
            *trailing_const = 1;
        }
    }
    return pointer_level;
}

int parser_pointer(parser_t *p)
{
    int trailing_const;
    return parser_pointer_ex(p, &trailing_const);
}

static node_t *parser_declarator_mode(parser_t *p, decl_mode_t mode)
{
    int pointer_level = 0;
    int trailing_const = 0;
    if (parser_peek(p)->type == '*') {
        pointer_level = parser_pointer_ex(p, &trailing_const);
        if (pointer_level <= 0)
            return NULL;
    }

    // In abstract/either mode, pointer alone (no direct part) is valid.
    // A direct_abstract_declarator starts with '(' or '['; TOKEN_IDENT starts a concrete one.
    if (mode != DECL_CONCRETE) {
        token_t *peek = parser_peek(p);
        if (peek->type != '(' && peek->type != '[' && peek->type != TOKEN_IDENT) {
            if (pointer_level == 0)
                return NULL; // nothing was parsed
            node_t *n = node_create(ND_DIRECT_DECL, peek->line, peek->col);
            n->direct_decl.ident.str = NULL;
            n->direct_decl.ident.len = 0;
            n->direct_decl.pointer_level = pointer_level;
            n->direct_decl.array_size = 0;
            n->direct_decl.is_const_qualified = trailing_const;
            n->direct_decl.param_list = NULL;
            return n;
        }
    }

    node_t *n = parser_direct_declarator(p, mode);
    if (n == NULL)
        return NULL;
    n->direct_decl.pointer_level += pointer_level;
    if (trailing_const)
        n->direct_decl.is_const_qualified = 1;
    return n;
}

static node_t *parser_declarator(parser_t *p)
{
    return parser_declarator_mode(p, DECL_CONCRETE);
}

static bool parser_is_type_token(parser_t *p, token_t *tok)
{
    token_type_t type = tok->type;
    if (type == TOKEN_KW_INT || type == TOKEN_KW_CHAR || type == TOKEN_KW_VOID ||
        type == TOKEN_KW_SHORT || type == TOKEN_KW_LONG ||
        type == TOKEN_KW_STRUCT || type == TOKEN_KW_UNION || type == TOKEN_KW_ENUM || type == TOKEN_KW_CONST)
        return true;
    if (type == TOKEN_IDENT && typedef_def_lookup(p->typedef_defs, tok->sval, tok->len))
        return true;
    return false;
}

/* Return the byte size of a type given its decl_spec and pointer level */
static int parser_sym_size(node_t *decl_spec, int pointer_level)
{
    if (pointer_level > 0)
        return 8;
    if (decl_spec == NULL || decl_spec->decl_spec.type_spec == NULL)
        return 4;
    if (decl_spec->decl_spec.type_spec->kind == ND_STRUCT_SPEC) {
        struct_def_t *def = decl_spec->decl_spec.type_spec->struct_spec.def;
        return def ? def->size : 4;
    }
    if (decl_spec->decl_spec.type_spec->kind == ND_ENUM_SPEC)
        return 4;
    switch (decl_spec->decl_spec.type_spec->type_spec) {
        case ND_TYPE_CHAR:
            return 1;
        case ND_TYPE_SHORT:
            return 2;
        case ND_TYPE_INT:
            return 4;
        case ND_TYPE_LONG:
            return 8;
        case ND_TYPE_VOID:
            return 1;
    }
    return 4;
}

/* Align offset (negative) to -align boundary going further negative.
 * e.g. parser_align_down(-1, 4) == -4, parser_align_down(-5, 4) == -8 */
static int parser_align_down(int offset, int align)
{
    /* offset is <= 0; we want the most-negative multiple of align >= offset */
    return -((-offset + align - 1) / align * align);
}

/* Parse an initializer: assignment_expression | '{' initializer_list '}'
 *
 * initializer
 *   : assignment_expression
 *   | '{' initializer_list [','] '}'
 *   ;
 */
static node_t *parser_initializer(parser_t *p)
{
    token_t *peek = parser_peek(p);
    if (peek->type != '{')
        return parser_assignment_expression(p);

    token_t *lb = parser_next(p);
    int line = lb->line, col = lb->col;
    token_destroy(lb);

    node_t *n = node_create(ND_INITIALIZER_LIST, line, col);
    n->initializer_list.count = 0;

    if (parser_peek(p)->type != '}') {
        while (1) {
            if (n->initializer_list.count >= 64) {
                fprintf(stderr, "%d:%d: error: too many initializers (max 64)\n", line, col);
                node_destroy(n);
                return NULL;
            }
            node_t *item = parser_initializer(p);
            if (item == NULL) {
                node_destroy(n);
                return NULL;
            }
            n->initializer_list.items[n->initializer_list.count++] = item;
            if (!parser_accept(p, ','))
                break;
            if (parser_peek(p)->type == '}')
                break; // trailing comma
        }
    }

    if (!parser_expect(p, '}')) {
        node_destroy(n);
        return NULL;
    }
    return n;
}

/* Parse a local variable declaration: decl_spec declarator [= expr] ;
 * Defines the symbol in the current scope and returns ND_LOCAL_DECL. */
static node_t *parser_local_declaration(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL)
        return NULL;

    node_t *declarator = parser_declarator(p);
    if (declarator == NULL) {
        node_destroy(decl_spec);
        return NULL;
    }

    // Handle block-scope typedef declaration
    if (decl_spec->decl_spec.storage_class == SC_TYPEDEF) {
        if (!parser_expect(p, ';')) {
            node_destroy(decl_spec);
            node_destroy(declarator);
            return NULL;
        }
        typedef_def_t *td = malloc(sizeof(typedef_def_t));
        td->name = declarator->direct_decl.ident.str;
        td->name_len = declarator->direct_decl.ident.len;
        td->decl_spec = decl_spec;
        td->pointer_level = declarator->direct_decl.pointer_level + decl_spec->decl_spec.pointer_level;
        td->next = p->typedef_defs;
        p->typedef_defs = td;

        node_t *n = node_create(ND_LOCAL_DECL, decl_spec->line, decl_spec->col);
        n->local_decl.decl_spec = decl_spec;
        n->local_decl.declarator = declarator;
        n->local_decl.init = NULL;
        n->local_decl.sym = NULL;
        return n;
    }

    int pointer_level = declarator->direct_decl.pointer_level + decl_spec->decl_spec.pointer_level;
    int array_size = declarator->direct_decl.array_size;
    int elem_size = parser_sym_size(decl_spec, pointer_level);
    int total_size;
    if (array_size > 0) {
        total_size = elem_size * array_size;
    } else if (array_size == -1) {
        fprintf(stderr, "%d:%d: error: array size missing in declaration\n", declarator->line,
                declarator->col);
        node_destroy(decl_spec);
        node_destroy(declarator);
        return NULL;
    } else {
        total_size = elem_size;
    }
    int align = elem_size < 8 ? elem_size : 8;
    if (pointer_level == 0 && decl_spec->decl_spec.type_spec &&
        decl_spec->decl_spec.type_spec->kind == ND_STRUCT_SPEC) {
        struct_def_t *def = decl_spec->decl_spec.type_spec->struct_spec.def;
        if (def)
            align = def->align;
    }

    int is_static_local = (decl_spec->decl_spec.storage_class == SC_STATIC);
    int is_extern_local = (decl_spec->decl_spec.storage_class == SC_EXTERN);
    sym_t *sym;
    if (is_extern_local) {
        // Extern locals: no stack allocation; accessed via %rip-relative using the symbol name
        sym = scope_define(p->scope, declarator->direct_decl.ident.str,
                           declarator->direct_decl.ident.len, decl_spec, pointer_level, array_size,
                           0);
        sym->is_global = 1;
        sym->is_extern = 1;
    } else if (is_static_local) {
        // Static locals: no stack allocation; use unique asm label, accessed via %rip-relative
        sym = scope_define(p->scope, declarator->direct_decl.ident.str,
                           declarator->direct_decl.ident.len, decl_spec, pointer_level, array_size,
                           0);
        sym->is_global = 1;
        sym->is_static = 1;
        char buf[128];
        int label_len = snprintf(buf, sizeof(buf), "%.*s.%d", declarator->direct_decl.ident.len,
                                 declarator->direct_decl.ident.str, p->static_local_count++);
        sym->asm_label = malloc((size_t)label_len + 1);
        memcpy(sym->asm_label, buf, (size_t)label_len + 1);
        sym->asm_label_len = label_len;
    } else {
        p->frame_offset = parser_align_down(p->frame_offset - total_size, align);
        sym = scope_define(p->scope, declarator->direct_decl.ident.str,
                           declarator->direct_decl.ident.len, decl_spec, pointer_level, array_size,
                           p->frame_offset);
    }

    // Propagate const qualifier to symbol
    if ((decl_spec->decl_spec.type_qualifier & TQ_CONST) && pointer_level == 0)
        sym->is_const = 1; // const scalar/struct/array
    if (declarator->direct_decl.is_const_qualified && pointer_level > 0)
        sym->is_const = 1; // const pointer: "int * const p"

    node_t *init = NULL;
    if (parser_accept(p, '=')) {
        init = parser_initializer(p);
        if (init == NULL) {
            p->scope->syms = sym->next;
            free(sym->asm_label);
            free(sym);
            node_destroy(decl_spec);
            node_destroy(declarator);
            return NULL;
        }
        // Extern locals cannot have initializers
        if (is_extern_local) {
            fprintf(stderr, "%d:%d: error: 'extern' variable cannot have an initializer\n",
                    init->line, init->col);
            p->scope->syms = sym->next;
            free(sym);
            node_destroy(decl_spec);
            node_destroy(declarator);
            node_destroy(init);
            return NULL;
        }
        // Static locals require constant initializers
        if (is_static_local && init->kind != ND_NUM && init->kind != ND_STR &&
            init->kind != ND_INITIALIZER_LIST) {
            fprintf(stderr, "%d:%d: error: initializer for static variable is not a constant\n",
                    init->line, init->col);
            p->scope->syms = sym->next;
            free(sym->asm_label);
            free(sym);
            node_destroy(decl_spec);
            node_destroy(declarator);
            node_destroy(init);
            return NULL;
        }
    }

    if (!parser_expect(p, ';')) {
        p->scope->syms = sym->next;
        free(sym->asm_label);
        free(sym);
        node_destroy(decl_spec);
        node_destroy(declarator);
        node_destroy(init);
        return NULL;
    }

    node_t *n = node_create(ND_LOCAL_DECL, decl_spec->line, decl_spec->col);
    n->local_decl.decl_spec = decl_spec;
    n->local_decl.declarator = declarator;
    n->local_decl.init = init;
    n->local_decl.sym = sym;
    return n;
}

// Parse a type name for cast / sizeof: declaration_specifiers + optional abstract_declarator
static node_t *parser_type_name(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL)
        return NULL;
    if (decl_spec->decl_spec.storage_class != SC_NONE) {
        fprintf(stderr, "%d:%d: error: storage class specifier not allowed in type name\n",
                decl_spec->line, decl_spec->col);
        node_destroy(decl_spec);
        return NULL;
    }

    // Parse optional abstract declarator (starts with '*', '(', or '[')
    token_t *peek = parser_peek(p);
    if (peek->type == '*' || peek->type == '(' || peek->type == '[') {
        node_t *abs_decl = parser_declarator_mode(p, DECL_ABSTRACT);
        if (abs_decl == NULL) {
            node_destroy(decl_spec);
            return NULL;
        }
        decl_spec->decl_spec.pointer_level = abs_decl->direct_decl.pointer_level;
        node_destroy(abs_decl);
    }
    return decl_spec;
}

/*
 * primary_expression
 * : IDENTIFIER
 * | CONSTANT
 * | STRING_LITERAL
 * | '(' expression ')'
 * ;
 */
static node_t *parser_primary_expression(parser_t *p)
{
    token_t *tok = parser_peek(p);
    if (tok == NULL)
        return NULL;

    if (tok->type == TOKEN_IDENT) {
        tok = parser_next(p);
        node_t *n = node_create(ND_IDENT, tok->line, tok->col);
        n->ident.name.str = tok->sval;
        n->ident.name.len = tok->len;
        if (p->scope) {
            n->ident.sym = scope_lookup(p->scope, tok->sval, tok->len);
            if (n->ident.sym == NULL)
                n->ident.sym = scope_lookup(p->func_scope, tok->sval, tok->len);
            if (n->ident.sym == NULL)
                n->ident.sym = scope_lookup(p->global_scope, tok->sval, tok->len);
            if (n->ident.sym == NULL)
                n->ident.sym = parser_enum_lookup(p, tok->sval, tok->len);
            if (n->ident.sym == NULL)
                fprintf(stderr, "%d:%d: warning: undeclared identifier '%.*s'\n", tok->line,
                        tok->col, tok->len, tok->sval);
        }
        token_destroy(tok);
        return n;
    }
    if (tok->type == TOKEN_NUM) {
        tok = parser_next(p);
        node_t *n = node_create(ND_NUM, tok->line, tok->col);
        n->num.val.str = tok->sval;
        n->num.val.len = tok->len;
        token_destroy(tok);
        return n;
    }
    if (tok->type == TOKEN_STR) {
        tok = parser_next(p);
        node_t *n = node_create(ND_STR, tok->line, tok->col);
        n->str.val.str = tok->sval;
        n->str.val.len = tok->len;
        token_destroy(tok);
        return n;
    }
    if (tok->type == '(') {
        token_destroy(parser_next(p)); // consume '('
        node_t *inner = parser_expression(p);
        if (inner == NULL)
            return NULL;
        if (!parser_expect(p, ')')) {
            node_destroy(inner);
            return NULL;
        }
        return inner;
    }
    fprintf(stderr, "Expected expression but received '%.*s'\n", tok->len, tok->sval);
    return NULL;
}

/* Try to resolve a struct member access. Sets mem->member.resolved if
 * the object is a simple ND_IDENT with a struct-typed symbol, or a
 * resolved ND_MEMBER (for chained access like a.b.c). */
static void parser_resolve_member(node_t *mem)
{
    mem->member.resolved = NULL;
    node_t *obj = mem->member.object;

    node_t *ds = NULL;
    int ptr_level = 0;

    if (obj->kind == ND_IDENT && obj->ident.sym != NULL) {
        ds = obj->ident.sym->decl_spec;
        ptr_level = obj->ident.sym->pointer_level;
    } else if (obj->kind == ND_MEMBER && obj->member.resolved != NULL) {
        ds = obj->member.resolved->decl_spec;
        ptr_level = obj->member.resolved->pointer_level;
    } else {
        return;
    }

    if (ds == NULL || ds->decl_spec.type_spec == NULL)
        return;

    // For '.': object must be a struct (pointer_level == 0)
    // For '->': object must be a pointer to struct (pointer_level > 0)
    int expected_ptr = mem->member.is_ptr ? 1 : 0;
    if ((ptr_level > 0) != expected_ptr)
        return;

    if (ds->decl_spec.type_spec->kind != ND_STRUCT_SPEC)
        return;
    struct_def_t *def = ds->decl_spec.type_spec->struct_spec.def;
    if (def == NULL)
        return;
    mem->member.resolved = struct_member_lookup(def, mem->member.field.str, mem->member.field.len);
}

/*
 * postfix_expression
 * : primary_expression
 * | postfix_expression '[' expression ']'
 * | postfix_expression '(' ')'
 * | postfix_expression '(' argument_expression_list ')'
 * | postfix_expression '.' IDENTIFIER
 * | postfix_expression PTR_OP IDENTIFIER
 * | postfix_expression INC_OP
 * | postfix_expression DEC_OP
 * ;
 */
static node_t *parser_postfix_expression(parser_t *p)
{
    node_t *node = parser_primary_expression(p);
    if (node == NULL)
        return NULL;

    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL)
            break;

        if (peek->type == '[') {
            token_t *op_tok = parser_next(p);
            int line = op_tok->line, col = op_tok->col;
            token_destroy(op_tok);
            node_t *index = parser_expression(p);
            if (index == NULL) {
                node_destroy(node);
                return NULL;
            }
            if (!parser_expect(p, ']')) {
                node_destroy(index);
                node_destroy(node);
                return NULL;
            }
            node_t *sub = node_create(ND_SUBSCRIPT, line, col);
            sub->subscript.array = node;
            sub->subscript.index = index;
            node = sub;

        } else if (peek->type == '(') {
            token_t *op_tok = parser_next(p);
            int line = op_tok->line, col = op_tok->col;
            token_destroy(op_tok);
            node_t *call = node_create(ND_CALL, line, col);
            call->call.func = node;
            call->call.nargs = 0;
            if (!parser_accept(p, ')')) {
                // Parse argument list
                node_t *arg = parser_assignment_expression(p);
                if (arg == NULL) {
                    node_destroy(call);
                    return NULL;
                }
                call->call.args[call->call.nargs++] = arg;
                while (parser_accept(p, ',')) {
                    arg = parser_assignment_expression(p);
                    if (arg == NULL) {
                        node_destroy(call);
                        return NULL;
                    }
                    if (call->call.nargs < 8)
                        call->call.args[call->call.nargs++] = arg;
                }
                if (!parser_expect(p, ')')) {
                    node_destroy(call);
                    return NULL;
                }
            }
            node = call;

        } else if (peek->type == '.') {
            token_t *op_tok = parser_next(p);
            int line = op_tok->line, col = op_tok->col;
            token_destroy(op_tok);
            token_t *field = parser_expect_token(p, TOKEN_IDENT);
            if (field == NULL) {
                node_destroy(node);
                return NULL;
            }
            node_t *mem = node_create(ND_MEMBER, line, col);
            mem->member.object = node;
            mem->member.field.str = field->sval;
            mem->member.field.len = field->len;
            mem->member.is_ptr = 0;
            mem->member.resolved = NULL;
            token_destroy(field);
            parser_resolve_member(mem);
            node = mem;

        } else if (peek->type == TOKEN_PTR_OP) {
            token_t *op_tok = parser_next(p);
            int line = op_tok->line, col = op_tok->col;
            token_destroy(op_tok);
            token_t *field = parser_expect_token(p, TOKEN_IDENT);
            if (field == NULL) {
                node_destroy(node);
                return NULL;
            }
            node_t *mem = node_create(ND_MEMBER, line, col);
            mem->member.object = node;
            mem->member.field.str = field->sval;
            mem->member.field.len = field->len;
            mem->member.is_ptr = 1;
            mem->member.resolved = NULL;
            token_destroy(field);
            parser_resolve_member(mem);
            node = mem;

        } else if (peek->type == TOKEN_INC_OP || peek->type == TOKEN_DEC_OP) {
            if (node->kind == ND_IDENT && node->ident.sym && node->ident.sym->is_const) {
                fprintf(stderr, "%d:%d: error: cannot modify const variable '%.*s'\n", peek->line,
                        peek->col, node->ident.name.len, node->ident.name.str);
                node_destroy(node);
                return NULL;
            }
            token_t *op_tok = parser_next(p);
            node_t *post = node_create(ND_POSTOP, op_tok->line, op_tok->col);
            post->postop.op = op_tok->type;
            post->postop.operand = node;
            token_destroy(op_tok);
            node = post;

        } else {
            break;
        }
    }
    return node;
}

/*
 * unary_operator : '&' | '*' | '+' | '-' | '~' | '!'
 * unary_expression
 * : postfix_expression
 * | INC_OP unary_expression
 * | DEC_OP unary_expression
 * | unary_operator cast_expression
 * | SIZEOF unary_expression
 * | SIZEOF '(' type_name ')'
 * ;
 */
static node_t *parser_unary_expression(parser_t *p)
{
    token_t *peek = parser_peek(p);
    if (peek == NULL)
        return NULL;

    if (peek->type == TOKEN_INC_OP || peek->type == TOKEN_DEC_OP) {
        token_t *op_tok = parser_next(p);
        node_t *operand = parser_unary_expression(p);
        if (operand == NULL) {
            token_destroy(op_tok);
            return NULL;
        }
        if (operand->kind == ND_IDENT && operand->ident.sym && operand->ident.sym->is_const) {
            fprintf(stderr, "%d:%d: error: cannot modify const variable '%.*s'\n", op_tok->line,
                    op_tok->col, operand->ident.name.len, operand->ident.name.str);
            token_destroy(op_tok);
            node_destroy(operand);
            return NULL;
        }
        node_t *n = node_create(ND_UNOP, op_tok->line, op_tok->col);
        n->unop.op = op_tok->type;
        n->unop.operand = operand;
        token_destroy(op_tok);
        return n;
    }

    if (peek->type == '&' || peek->type == '*' || peek->type == '+' || peek->type == '-' ||
        peek->type == '~' || peek->type == '!') {
        token_t *op_tok = parser_next(p);
        node_t *operand = parser_cast_expression(p);
        if (operand == NULL) {
            token_destroy(op_tok);
            return NULL;
        }
        node_t *n = node_create(ND_UNOP, op_tok->line, op_tok->col);
        n->unop.op = op_tok->type;
        n->unop.operand = operand;
        token_destroy(op_tok);
        return n;
    }

    if (peek->type == TOKEN_KW_SIZEOF) {
        token_t *kw = parser_next(p);
        int line = kw->line, col = kw->col;
        token_destroy(kw);
        // Check for sizeof '(' type_name ')'
        token_t *p1 = parser_peek(p);
        if (p1 != NULL && p1->type == '(') {
            token_t *p2 = parser_peek2(p);
            if (p2 != NULL && parser_is_type_token(p, p2)) {
                token_destroy(parser_next(p)); // consume '('
                node_t *type_node = parser_type_name(p);
                if (type_node == NULL)
                    return NULL;
                if (!parser_expect(p, ')')) {
                    node_destroy(type_node);
                    return NULL;
                }
                node_t *n = node_create(ND_SIZEOF_TYPE, line, col);
                n->sizeof_type.type_node = type_node;
                return n;
            }
        }
        node_t *operand = parser_unary_expression(p);
        if (operand == NULL)
            return NULL;
        node_t *n = node_create(ND_SIZEOF_EXPR, line, col);
        n->sizeof_expr.expr = operand;
        return n;
    }

    return parser_postfix_expression(p);
}

/*
 * cast_expression
 * : unary_expression
 * | '(' type_name ')' cast_expression
 * ;
 */
static node_t *parser_cast_expression(parser_t *p)
{
    token_t *p1 = parser_peek(p);
    if (p1 != NULL && p1->type == '(') {
        token_t *p2 = parser_peek2(p);
        if (p2 != NULL && parser_is_type_token(p, p2)) {
            token_t *lp = parser_next(p); // consume '('
            int line = lp->line, col = lp->col;
            token_destroy(lp);
            node_t *type_node = parser_type_name(p);
            if (type_node == NULL)
                return NULL;
            if (!parser_expect(p, ')')) {
                node_destroy(type_node);
                return NULL;
            }
            node_t *expr = parser_cast_expression(p);
            if (expr == NULL) {
                node_destroy(type_node);
                return NULL;
            }
            node_t *n = node_create(ND_CAST, line, col);
            n->cast.type_node = type_node;
            n->cast.expr = expr;
            return n;
        }
    }
    return parser_unary_expression(p);
}

/*
 * multiplicative_expression
 * : cast_expression
 * | multiplicative_expression '*' cast_expression
 * | multiplicative_expression '/' cast_expression
 * | multiplicative_expression '%' cast_expression
 * ;
 */
static node_t *parser_multiplicative_expression(parser_t *p)
{
    node_t *node = parser_cast_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL)
            break;
        if (peek->type != '*' && peek->type != '/' && peek->type != '%')
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_cast_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * additive_expression
 * : multiplicative_expression
 * | additive_expression '+' multiplicative_expression
 * | additive_expression '-' multiplicative_expression
 * ;
 */
static node_t *parser_additive_expression(parser_t *p)
{
    node_t *node = parser_multiplicative_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL)
            break;
        if (peek->type != '+' && peek->type != '-')
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_multiplicative_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * shift_expression
 * : additive_expression
 * | shift_expression LEFT_OP additive_expression
 * | shift_expression RIGHT_OP additive_expression
 * ;
 */
static node_t *parser_shift_expression(parser_t *p)
{
    node_t *node = parser_additive_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL)
            break;
        if (peek->type != TOKEN_LEFT_OP && peek->type != TOKEN_RIGHT_OP)
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_additive_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * relational_expression
 * : shift_expression
 * | relational_expression '<' shift_expression
 * | relational_expression '>' shift_expression
 * | relational_expression LE_OP shift_expression
 * | relational_expression GE_OP shift_expression
 * ;
 */
static node_t *parser_relational_expression(parser_t *p)
{
    node_t *node = parser_shift_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL)
            break;
        if (peek->type != '<' && peek->type != '>' && peek->type != TOKEN_LE_OP &&
            peek->type != TOKEN_GE_OP)
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_shift_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * equality_expression
 * : relational_expression
 * | equality_expression EQ_OP relational_expression
 * | equality_expression NE_OP relational_expression
 * ;
 */
static node_t *parser_equality_expression(parser_t *p)
{
    node_t *node = parser_relational_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL)
            break;
        if (peek->type != TOKEN_EQ_OP && peek->type != TOKEN_NE_OP)
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_relational_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * and_expression
 * : equality_expression
 * | and_expression '&' equality_expression
 * ;
 */
static node_t *parser_and_expression(parser_t *p)
{
    node_t *node = parser_equality_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL || peek->type != '&')
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_equality_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * exclusive_or_expression
 * : and_expression
 * | exclusive_or_expression '^' and_expression
 * ;
 */
static node_t *parser_exclusive_or_expression(parser_t *p)
{
    node_t *node = parser_and_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL || peek->type != '^')
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_and_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * inclusive_or_expression
 * : exclusive_or_expression
 * | inclusive_or_expression '|' exclusive_or_expression
 * ;
 */
static node_t *parser_inclusive_or_expression(parser_t *p)
{
    node_t *node = parser_exclusive_or_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL || peek->type != '|')
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_exclusive_or_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * logical_and_expression
 * : inclusive_or_expression
 * | logical_and_expression AND_OP inclusive_or_expression
 * ;
 */
static node_t *parser_logical_and_expression(parser_t *p)
{
    node_t *node = parser_inclusive_or_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL || peek->type != TOKEN_AND_OP)
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_inclusive_or_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * logical_or_expression
 * : logical_and_expression
 * | logical_or_expression OR_OP logical_and_expression
 * ;
 */
static node_t *parser_logical_or_expression(parser_t *p)
{
    node_t *node = parser_logical_and_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL || peek->type != TOKEN_OR_OP)
            break;
        token_t *op_tok = parser_next(p);
        int op = op_tok->type, line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_logical_and_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *binop = node_create(ND_BINOP, line, col);
        binop->binop.op = op;
        binop->binop.left = node;
        binop->binop.right = right;
        node = binop;
    }
    return node;
}

/*
 * conditional_expression
 * : logical_or_expression
 * | logical_or_expression '?' expression ':' conditional_expression
 * ;
 */
static node_t *parser_conditional_expression(parser_t *p)
{
    node_t *cond = parser_logical_or_expression(p);
    if (cond == NULL)
        return NULL;
    token_t *peek = parser_peek(p);
    if (peek == NULL || peek->type != '?')
        return cond;
    token_t *op_tok = parser_next(p); // consume '?'
    int line = op_tok->line, col = op_tok->col;
    token_destroy(op_tok);
    node_t *then_expr = parser_expression(p);
    if (then_expr == NULL) {
        node_destroy(cond);
        return NULL;
    }
    if (!parser_expect(p, ':')) {
        node_destroy(cond);
        node_destroy(then_expr);
        return NULL;
    }
    node_t *else_expr = parser_conditional_expression(p);
    if (else_expr == NULL) {
        node_destroy(cond);
        node_destroy(then_expr);
        return NULL;
    }
    node_t *n = node_create(ND_TERNARY, line, col);
    n->ternary.cond = cond;
    n->ternary.then_expr = then_expr;
    n->ternary.else_expr = else_expr;
    return n;
}

static bool parser_is_assign_op(token_type_t type)
{
    return type == '=' || type == TOKEN_MUL_ASSIGN || type == TOKEN_DIV_ASSIGN ||
           type == TOKEN_MOD_ASSIGN || type == TOKEN_ADD_ASSIGN || type == TOKEN_SUB_ASSIGN ||
           type == TOKEN_LEFT_ASSIGN || type == TOKEN_RIGHT_ASSIGN || type == TOKEN_AND_ASSIGN ||
           type == TOKEN_XOR_ASSIGN || type == TOKEN_OR_ASSIGN;
}

/*
 * assignment_operator: '=' | MUL_ASSIGN | ... | OR_ASSIGN
 * assignment_expression
 * : conditional_expression
 * | unary_expression assignment_operator assignment_expression
 * ;
 */
static node_t *parser_assignment_expression(parser_t *p)
{
    node_t *lhs = parser_conditional_expression(p);
    if (lhs == NULL)
        return NULL;
    token_t *peek = parser_peek(p);
    if (peek == NULL || !parser_is_assign_op(peek->type))
        return lhs;
    token_t *op_tok = parser_next(p);
    int op = op_tok->type, line = op_tok->line, col = op_tok->col;
    token_destroy(op_tok);
    if (lhs->kind == ND_IDENT && lhs->ident.sym && lhs->ident.sym->is_const) {
        fprintf(stderr, "%d:%d: error: cannot assign to const variable '%.*s'\n", line, col,
                lhs->ident.name.len, lhs->ident.name.str);
        node_destroy(lhs);
        return NULL;
    }
    node_t *rhs = parser_assignment_expression(p); // right-recursive
    if (rhs == NULL) {
        node_destroy(lhs);
        return NULL;
    }
    node_t *n = node_create(ND_ASSIGN, line, col);
    n->assign.op = op;
    n->assign.lhs = lhs;
    n->assign.rhs = rhs;
    return n;
}

/*
 * expression
 * : assignment_expression
 * | expression ',' assignment_expression
 * ;
 */
static node_t *parser_expression(parser_t *p)
{
    node_t *node = parser_assignment_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL || peek->type != ',')
            break;
        token_t *op_tok = parser_next(p);
        int line = op_tok->line, col = op_tok->col;
        token_destroy(op_tok);
        node_t *right = parser_assignment_expression(p);
        if (right == NULL) {
            node_destroy(node);
            return NULL;
        }
        node_t *comma = node_create(ND_COMMA, line, col);
        comma->comma.left = node;
        comma->comma.right = right;
        node = comma;
    }
    return node;
}

static node_t *parser_expression_statement(parser_t *p)
{
    token_t *peek = parser_peek(p);
    node_t *n = node_create(ND_EXPR_STMT, peek->line, peek->col);
    if (parser_accept(p, ';')) {
        n->expr_stmt.expr = NULL;
        return n;
    }
    node_t *expr = parser_expression(p);
    if (expr == NULL) {
        node_destroy(n);
        return NULL;
    }
    if (!parser_expect(p, ';')) {
        node_destroy(expr);
        node_destroy(n);
        return NULL;
    }
    n->expr_stmt.expr = expr;
    return n;
}

/*
 * selection_statement
 *  : IF '(' expression ')' statement
 *  | IF '(' expression ')' statement ELSE statement
 *  | SWITCH '(' expression ')' statement
 *  ;
 */
static node_t *parser_selection_statement(parser_t *p)
{
    token_t *t = parser_next(p);
    int line = t->line, col = t->col;
    token_type_t ttype = t->type;
    token_destroy(t);
    if (ttype == TOKEN_KW_IF) {
        if (!parser_expect(p, '('))
            return NULL;
        node_t *cond = parser_expression(p);
        if (!cond)
            return NULL;
        if (!parser_expect(p, ')')) {
            node_destroy(cond);
            return NULL;
        }

        node_t *then = parser_statement(p);
        if (!then) {
            node_destroy(cond);
            return NULL;
        }

        node_t *else_ = NULL;
        if (parser_peek(p)->type == TOKEN_KW_ELSE) {
            token_destroy(parser_next(p)); // consume 'else'
            else_ = parser_statement(p);
            if (!else_) {
                node_destroy(cond);
                node_destroy(then);
                return NULL;
            }
        }

        node_t *n = node_create(ND_IF_STMT, line, col);
        n->if_stmt.cond = cond;
        n->if_stmt.then = then;
        n->if_stmt.else_ = else_;
        return n;
    } else if (ttype == TOKEN_KW_SWITCH) {
        if (!parser_expect(p, '('))
            return NULL;
        node_t *expr = parser_expression(p);
        if (!expr)
            return NULL;
        if (!parser_expect(p, ')')) {
            node_destroy(expr);
            return NULL;
        }
        node_t *body = parser_statement(p);
        if (!body) {
            node_destroy(expr);
            return NULL;
        }
        node_t *n = node_create(ND_SWITCH_STMT, line, col);
        n->switch_stmt.expr = expr;
        n->switch_stmt.body = body;
        return n;
    }
    return NULL;
}

/*
 * iteration_statement
 *  : WHILE '(' expression ')' statement
 *  | DO statement WHILE '(' expression ')' ';'
 *  | FOR '(' expression_statement expression_statement ')' statement
 *  | FOR '(' expression_statement expression_statement expression ')' statement
 *  ;
 */
static node_t *parser_iteration_statement(parser_t *p)
{
    token_t *t = parser_peek(p);
    if (t->type == TOKEN_KW_WHILE) {
        t = parser_next(p); // consume 'while'
        int line = t->line, col = t->col;
        token_destroy(t);
        if (!parser_expect(p, '('))
            return NULL;
        node_t *cond = parser_expression(p);
        if (!cond)
            return NULL;
        if (!parser_expect(p, ')'))
            return NULL;

        node_t *body = parser_statement(p);
        if (!body) {
            node_destroy(cond);
            return NULL;
        }

        node_t *n = node_create(ND_WHILE_STMT, line, col);
        n->while_stmt.cond = cond;
        n->while_stmt.body = body;
        return n;
    } else if (t->type == TOKEN_KW_DO) {
        t = parser_next(p); // consume 'do'
        int line = t->line, col = t->col;
        token_destroy(t);

        node_t *body = parser_statement(p);
        if (!body)
            return NULL;
        if (!parser_expect(p, TOKEN_KW_WHILE)) {
            node_destroy(body);
            return NULL;
        }
        if (!parser_expect(p, '(')) {
            node_destroy(body);
            return NULL;
        }
        node_t *cond = parser_expression(p);
        if (!cond) {
            node_destroy(body);
            return NULL;
        }
        if (!parser_expect(p, ')') || !parser_expect(p, ';')) {
            node_destroy(cond);
            node_destroy(body);
            return NULL;
        }
        node_t *n = node_create(ND_DO_WHILE_STMT, line, col);
        n->while_stmt.cond = cond;
        n->while_stmt.body = body;
        return n;
    } else if (t->type == TOKEN_KW_FOR) {
        t = parser_next(p); // consume 'for'
        int line = t->line, col = t->col;
        token_destroy(t);

        if (!parser_expect(p, '('))
            return NULL;

        node_t *init = parser_expression_statement(p);
        if (!init)
            return NULL;

        node_t *cond = NULL;
        if (!parser_accept(p, ';')) {
            cond = parser_expression(p);
            if (!cond) {
                node_destroy(init);
                return NULL;
            }
            if (!parser_expect(p, ';')) {
                node_destroy(cond);
                node_destroy(init);
                return NULL;
            }
        }

        node_t *post = NULL;
        if (parser_peek(p)->type != ')') {
            post = parser_expression(p);
            if (!post) {
                node_destroy(cond);
                node_destroy(init);
                return NULL;
            }
        }

        if (!parser_expect(p, ')')) {
            node_destroy(post);
            node_destroy(cond);
            node_destroy(init);
            return NULL;
        }

        node_t *body = parser_statement(p);
        if (!body) {
            node_destroy(post);
            node_destroy(cond);
            node_destroy(init);
            return NULL;
        }

        node_t *n = node_create(ND_FOR_STMT, line, col);
        n->for_stmt.init = init;
        n->for_stmt.cond = cond;
        n->for_stmt.post = post;
        n->for_stmt.body = body;
        return n;
    }
    return NULL;
}

static node_t *parser_jump_statement(parser_t *p)
{
    token_t *t = parser_peek(p);
    if (t->type == TOKEN_KW_BREAK) {
        t = parser_next(p);
        node_t *n = node_create(ND_BREAK_STMT, t->line, t->col);
        token_destroy(t);
        if (!parser_expect(p, ';')) {
            node_destroy(n);
            return NULL;
        }
        return n;
    } else if (t->type == TOKEN_KW_CONTINUE) {
        t = parser_next(p);
        node_t *n = node_create(ND_CONTINUE_STMT, t->line, t->col);
        token_destroy(t);
        if (!parser_expect(p, ';')) {
            node_destroy(n);
            return NULL;
        }
        return n;
    }
    token_t *ret_tok = parser_expect_token(p, TOKEN_KW_RETURN);
    if (ret_tok == NULL)
        return NULL;
    node_t *n = node_create(ND_RETURN_STMT, ret_tok->line, ret_tok->col);
    token_destroy(ret_tok);
    if (parser_accept(p, ';')) {
        n->return_stmt.expr = NULL;
        return n;
    }
    node_t *expr = parser_expression(p);
    if (expr == NULL) {
        node_destroy(n);
        return NULL;
    }
    if (!parser_expect(p, ';')) {
        node_destroy(expr);
        node_destroy(n);
        return NULL;
    }
    n->return_stmt.expr = expr;
    return n;
}

/*
 * labeled_statement
 *  : CASE constant_expression ':' statement
 *  | DEFAULT ':' statement
 *  ;
 */
static node_t *parser_labeled_statement(parser_t *p)
{
    token_t *t = parser_next(p);
    int line = t->line, col = t->col;
    token_type_t ttype = t->type;
    token_destroy(t);

    if (ttype == TOKEN_KW_CASE) {
        node_t *expr = parser_conditional_expression(p);
        if (!expr)
            return NULL;
        if (!parser_expect(p, ':')) {
            node_destroy(expr);
            return NULL;
        }
        node_t *stmt = parser_statement(p);
        if (!stmt) {
            node_destroy(expr);
            return NULL;
        }
        node_t *n = node_create(ND_CASE_LABEL, line, col);
        n->case_label.expr = expr;
        n->case_label.stmt = stmt;
        return n;
    } else { // TOKEN_KW_DEFAULT
        if (!parser_expect(p, ':'))
            return NULL;
        node_t *stmt = parser_statement(p);
        if (!stmt)
            return NULL;
        node_t *n = node_create(ND_DEFAULT_LABEL, line, col);
        n->default_label.stmt = stmt;
        return n;
    }
}

static node_t *parser_statement(parser_t *p)
{
    token_t *t = parser_peek(p);
    if (t->type == '{') {
        return parser_compound_statement(p);
    } else if (t->type == TOKEN_KW_IF || t->type == TOKEN_KW_SWITCH) {
        return parser_selection_statement(p);
    } else if (t->type == TOKEN_KW_WHILE || t->type == TOKEN_KW_DO || t->type == TOKEN_KW_FOR) {
        return parser_iteration_statement(p);
    } else if (t->type == TOKEN_KW_RETURN || t->type == TOKEN_KW_BREAK ||
               t->type == TOKEN_KW_CONTINUE) {
        return parser_jump_statement(p);
    } else if (t->type == TOKEN_KW_CASE || t->type == TOKEN_KW_DEFAULT) {
        return parser_labeled_statement(p);
    } else {
        return parser_expression_statement(p);
    }
}

static bool parser_block_item_list(parser_t *p, node_t *comp)
{
    while (parser_peek(p)->type != '}') {
        node_t *item;
        if (parser_is_type_token(p, parser_peek(p)) ||
            parser_peek(p)->type == TOKEN_KW_STATIC ||
            parser_peek(p)->type == TOKEN_KW_EXTERN ||
            parser_peek(p)->type == TOKEN_KW_TYPEDEF)
            item = parser_local_declaration(p);
        else
            item = parser_statement(p);
        if (item == NULL)
            return false;
        comp->comp_stmt.stmts[comp->comp_stmt.nstmts++] = item;
    }
    return true;
}

static node_t *parser_compound_statement(parser_t *p)
{
    token_t *lbrace = parser_expect_token(p, '{');
    if (lbrace == NULL)
        return NULL;
    node_t *n = node_create(ND_COMP_STMT, lbrace->line, lbrace->col);
    token_destroy(lbrace);

    // Push a child scope for this block (nested in the function scope)
    scope_t *parent_scope = p->scope;
    if (parent_scope != NULL)
        p->scope = scope_new(parent_scope);

    if (!parser_block_item_list(p, n)) {
        if (p->scope != parent_scope) {
            scope_free(p->scope);
            p->scope = parent_scope;
        }
        node_destroy(n);
        return NULL;
    }

    // Pop the child scope; any sym_t nodes for locals are owned by
    // ND_LOCAL_DECL
    if (p->scope != parent_scope) {
        scope_free(p->scope);
        p->scope = parent_scope;
    }

    if (!parser_expect(p, '}')) {
        node_destroy(n);
        return NULL;
    }
    return n;
}

static node_t *parser_function_definition(parser_t *p, node_t *decl_spec, node_t *declarator)
{
    // Set up symbol table scope for the function body
    p->frame_offset = 0;
    p->scope = scope_new(NULL);

    // Register parameters in the function scope
    node_t *param_list = declarator->direct_decl.param_list;
    if (param_list != NULL) {
        for (int i = 0; i < param_list->param_list.nparams; i++) {
            node_t *pd = param_list->param_list.params[i];
            if (pd->param_decl.declarator == NULL ||
                pd->param_decl.declarator->direct_decl.ident.str == NULL) {
                fprintf(stderr, "%d:%d: error: parameter name omitted in function definition\n",
                        pd->line, pd->col);
                scope_free(p->scope);
                p->scope = NULL;
                node_destroy(decl_spec);
                node_destroy(declarator);
                return NULL;
            }
            int ptr_lvl = pd->param_decl.declarator->direct_decl.pointer_level +
                          pd->param_decl.decl_spec->decl_spec.pointer_level;
            int arr_sz = pd->param_decl.declarator->direct_decl.array_size;
            // Array parameters decay to pointers per C standard
            if (arr_sz != 0) {
                ptr_lvl += 1;
                arr_sz = 0;
            }
            int size = parser_sym_size(pd->param_decl.decl_spec, ptr_lvl);
            p->frame_offset = parser_align_down(p->frame_offset - size, size < 8 ? size : 8);
            sym_t *param_sym =
                scope_define(p->scope, pd->param_decl.declarator->direct_decl.ident.str,
                             pd->param_decl.declarator->direct_decl.ident.len,
                             pd->param_decl.decl_spec, ptr_lvl, arr_sz, p->frame_offset);
            if ((pd->param_decl.decl_spec->decl_spec.type_qualifier & TQ_CONST) && ptr_lvl == 0)
                param_sym->is_const = 1;
            if (pd->param_decl.declarator->direct_decl.is_const_qualified && ptr_lvl > 0)
                param_sym->is_const = 1;
        }
    }

    node_t *comp_stmt = parser_compound_statement(p);
    if (comp_stmt == NULL) {
        sym_destroy_list(p->scope->syms);
        scope_free(p->scope);
        p->scope = NULL;
        node_destroy(decl_spec);
        node_destroy(declarator);
        return NULL;
    }

    // Round frame size up to a multiple of 16 (x86-64 ABI requirement)
    int frame_size = -p->frame_offset;
    frame_size = (frame_size + 15) & ~15;

    sym_t *params_sym_list = p->scope->syms;
    p->scope->syms = NULL; // detach before freeing scope struct
    scope_free(p->scope);
    p->scope = NULL;

    node_t *node = node_create(ND_FUNC, decl_spec->line, decl_spec->col);
    node->func.decl_spec = decl_spec;
    node->func.declarator = declarator;
    node->func.comp_stmt = comp_stmt;
    node->func.frame_size = frame_size;
    node->func.params_sym_list = params_sym_list;

    // Register the function name in the global function scope so that calls
    // to this function after its definition resolve without a warning.
    node_str_t fname = declarator->direct_decl.ident;
    scope_define(p->func_scope, fname.str, fname.len, decl_spec,
                 declarator->direct_decl.pointer_level, 0, 0);

    return node;
}

/*
 * init_declarator
 *  : declarator
 *  | declarator '=' assignment_expression
 *  ;
 *
 * declaration
 *  : declaration_specifiers ';'
 *  | declaration_specifiers init_declarator_list ';'
 *  ;
 */
static node_t *parser_declaration_body(parser_t *p, node_t *decl_spec, node_t *declarator)
{
    node_t *init = NULL;
    if (parser_accept(p, '=')) {
        init = parser_initializer(p);
        if (init == NULL) {
            node_destroy(decl_spec);
            node_destroy(declarator);
            return NULL;
        }
    }
    // Extern variables cannot have initializers
    if (decl_spec->decl_spec.storage_class == SC_EXTERN && init != NULL) {
        fprintf(stderr, "%d:%d: error: 'extern' variable cannot have an initializer\n", init->line,
                init->col);
        node_destroy(decl_spec);
        node_destroy(declarator);
        node_destroy(init);
        return NULL;
    }
    if (!parser_expect(p, ';')) {
        node_destroy(decl_spec);
        node_destroy(declarator);
        node_destroy(init);
        return NULL;
    }
    node_t *n = node_create(ND_GLOBAL_DECL, decl_spec->line, decl_spec->col);
    n->global_decl.decl_spec = decl_spec;
    n->global_decl.declarator = declarator;
    n->global_decl.init = init;
    if (declarator != NULL && declarator->direct_decl.param_list == NULL) {
        // Variable declaration (not a forward function declaration)
        int pointer_level = declarator->direct_decl.pointer_level + decl_spec->decl_spec.pointer_level;
        int array_size = declarator->direct_decl.array_size;
        sym_t *sym = scope_define(p->global_scope, declarator->direct_decl.ident.str,
                                  declarator->direct_decl.ident.len, decl_spec, pointer_level,
                                  array_size, 0);
        sym->is_global = 1;
        if (decl_spec->decl_spec.storage_class == SC_STATIC)
            sym->is_static = 1;
        else if (decl_spec->decl_spec.storage_class == SC_EXTERN)
            sym->is_extern = 1;
        if ((decl_spec->decl_spec.type_qualifier & TQ_CONST) && pointer_level == 0)
            sym->is_const = 1;
        if (declarator->direct_decl.is_const_qualified && pointer_level > 0)
            sym->is_const = 1;
        n->global_decl.sym = sym;
    } else {
        n->global_decl.sym = NULL;
    }
    return n;
}

/*
 * external_declaration
 *  : function_definition
 *  | declaration
 *  ;
 *
 * Both alternatives start with declaration_specifiers declarator, so we parse
 * the common prefix once and branch on the token that follows the declarator:
 * '{' means a function body is next (function_definition); anything else is a
 * declaration.
 */
static node_t *parser_external_declaration(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL)
        return NULL;

    // declaration_specifiers ';' — bare type declaration (e.g. struct definition)
    if (parser_accept(p, ';')) {
        node_t *n = node_create(ND_GLOBAL_DECL, decl_spec->line, decl_spec->col);
        n->global_decl.decl_spec = decl_spec;
        n->global_decl.declarator = NULL;
        n->global_decl.init = NULL;
        return n;
    }

    node_t *declarator = parser_declarator(p);
    if (declarator == NULL) {
        node_destroy(decl_spec);
        return NULL;
    }

    // Handle typedef declaration: register the type alias and return a no-op node
    if (decl_spec->decl_spec.storage_class == SC_TYPEDEF) {
        if (!parser_expect(p, ';')) {
            node_destroy(decl_spec);
            node_destroy(declarator);
            return NULL;
        }
        typedef_def_t *td = malloc(sizeof(typedef_def_t));
        td->name = declarator->direct_decl.ident.str;
        td->name_len = declarator->direct_decl.ident.len;
        td->decl_spec = decl_spec;
        td->pointer_level = declarator->direct_decl.pointer_level + decl_spec->decl_spec.pointer_level;
        td->next = p->typedef_defs;
        p->typedef_defs = td;

        node_t *n = node_create(ND_GLOBAL_DECL, decl_spec->line, decl_spec->col);
        n->global_decl.decl_spec = decl_spec;
        n->global_decl.declarator = declarator;
        n->global_decl.init = NULL;
        n->global_decl.sym = NULL;
        return n;
    }

    token_t *peek = parser_peek(p);
    if (peek != NULL && peek->type == '{')
        return parser_function_definition(p, decl_spec, declarator);
    else
        return parser_declaration_body(p, decl_spec, declarator);
}

/*
 * translation_unit
 *  : external_declaration
 *  | translation_unit external_declaration
 *  ;
 */
static node_t *parser_translation_unit(parser_t *p)
{
    node_t *tu = node_create(ND_TRANSLATION_UNIT, 0, 0);
    while (parser_peek(p) != NULL) {
        node_t *item = parser_external_declaration(p);
        if (item == NULL) {
            node_destroy(tu);
            return NULL;
        }
        if (item->kind == ND_FUNC)
            tu->translation_unit.funcs[tu->translation_unit.nfuncs++] = item;
        else
            tu->translation_unit.decls[tu->translation_unit.ndecls++] = item;
    }
    if (tu->translation_unit.nfuncs == 0 && tu->translation_unit.ndecls == 0) {
        node_destroy(tu);
        return NULL;
    }
    return tu;
}

node_t *parser_exec(parser_t *p)
{
    return parser_translation_unit(p);
}
