#include "parser.h"
#include "error.h"

enum storage_class_spec {
    STORAGE_CLASS_INVALID = 0,
    STORAGE_CLASS_TYPEDEF = (1 << 0),
    STORAGE_CLASS_EXTERN = (1 << 1),
    STORAGE_CLASS_STATIC = (1 << 2),
    STORAGE_CLASS_AUTO = (1 << 3),
    STORAGE_CLASS_REGISTER = (1 << 4)
};

enum type_specifier {
    TYPE_SPEC_INVALID = 0,
    TYPE_SPEC_VOID = (1 << 0),
    TYPE_SPEC_CHAR = (1 << 1),
    TYPE_SPEC_SHORT = (1 << 2),
    TYPE_SPEC_INT = (1 << 3),
    TYPE_SPEC_LONG = (1 << 4),
    TYPE_SPEC_FLOAT = (1 << 5),
    TYPE_SPEC_DOUBLE = (1 << 6),
    TYPE_SPEC_SIGNED = (1 << 7),
    TYPE_SPEC_UNSIGNED = (1 << 8)
};

enum type_qualifier {
    TYPE_QUALI_INVALID = 0,
    TYPE_QUALI_CONST = (1 << 0),
    TYPE_QUALI_VOLATILE = (1 << 1)
};

typedef struct {
    int storage_class;
    int specifier;
    int qualifier;
} type_t;

static token_t *parser_next_token(parser_t *p)
{
    if (p->saved_tok) {
        token_t *t = p->saved_tok;
        p->saved_tok = NULL;
        return t;
    }
    return lex_next_token(&p->l);
}

static void parser_save_token(parser_t *p, token_t *t)
{
    p->saved_tok = t;
}

static enum storage_class_spec parser_storage_class_spec(token_t *t)
{
    switch (t->type) {
        case TOKEN_KW_TYPEDEF:
            return STORAGE_CLASS_TYPEDEF;
        case TOKEN_KW_EXTERN:
            return STORAGE_CLASS_EXTERN;
        case TOKEN_KW_STATIC:
            return STORAGE_CLASS_STATIC;
        case TOKEN_KW_AUTO:
            return STORAGE_CLASS_AUTO;
        case TOKEN_KW_REGISTER:
            return STORAGE_CLASS_REGISTER;
        default:
            return STORAGE_CLASS_INVALID;
    }
}

static enum type_specifier parser_type_specifier(token_t *t)
{
    switch (t->type) {
        case TOKEN_KW_VOID:
            return TYPE_SPEC_VOID;
        case TOKEN_KW_CHAR:
            return TYPE_SPEC_CHAR;
        case TOKEN_KW_SHORT:
            return TYPE_SPEC_SHORT;
        case TOKEN_KW_INT:
            return TYPE_SPEC_INT;
        case TOKEN_KW_LONG:
            return TYPE_SPEC_LONG;
        case TOKEN_KW_FLOAT:
            return TYPE_SPEC_FLOAT;
        case TOKEN_KW_DOUBLE:
            return TYPE_SPEC_DOUBLE;
        case TOKEN_KW_SIGNED:
            return TYPE_SPEC_SIGNED;
        case TOKEN_KW_UNSIGNED:
            return TYPE_SPEC_UNSIGNED;
        default:
            return TYPE_SPEC_INVALID;
    }
}

static enum type_qualifier parser_type_qualifier(token_t *t)
{
    switch (t->type) {
        case TOKEN_KW_CONST:
            return TYPE_QUALI_CONST;
        case TOKEN_KW_VOLATILE:
            return TYPE_QUALI_VOLATILE;
        default:
            return TYPE_QUALI_INVALID;
    }
}

static type_t parser_declaration_specifiers(parser_t *p)
{
    enum storage_class_spec sc;
    enum type_specifier ts;
    enum type_qualifier tq;
    type_t type = {0, 0, 0};
    for (;;) {
        token_t *tok = parser_next_token(p);
        if (tok == NULL)
            ccerror("end of file\n");
        sc = parser_storage_class_spec(tok);
        if (sc != STORAGE_CLASS_INVALID) {
            if (type.storage_class & sc)
                ccerror("duplicate storage class");
            type.storage_class |= sc;
            token_destroy(tok);
            continue;
        }
        ts = parser_type_specifier(tok);
        if (ts != TYPE_SPEC_INVALID) {
            if (type.specifier & ts)
                ccerror("duplicate type specifier");
            type.specifier |= ts;
            token_destroy(tok);
            continue;
        }
        tq = parser_type_qualifier(tok);
        if (tq != TYPE_QUALI_INVALID) {
            if (type.qualifier & tq)
                ccerror("duplicate type qualifier");
            type.qualifier |= tq;
            token_destroy(tok);
            continue;
        }
        if (!(sc | ts | tq)) {
            parser_save_token(p, tok);
            break;
        }
    }
    if (!(type.storage_class | type.specifier | type.qualifier)) {
        ccerror("invalid declaration specifiers");
    }
    return type;
}

int parser_init(parser_t *p, FILE *input)
{
    lex_init(&p->l, input);
    p->saved_tok = NULL;
    return 0;
}

void parser_next(parser_t *p)
{
    for (;;) {
        type_t type = parser_declaration_specifiers(p);
    }
}
