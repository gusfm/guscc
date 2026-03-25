#include "parser.h"

#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "sym.h"

// Forward declarations
node_t *parser_declarator(parser_t *p);
node_t *parser_compound_statement(parser_t *p);
node_t *parser_statement(parser_t *p);
node_t *parser_declaration_specifiers(parser_t *p);
node_t *parser_expression(parser_t *p);
node_t *parser_assignment_expression(parser_t *p);
node_t *parser_cast_expression(parser_t *p);
node_t *parser_unary_expression(parser_t *p);

void parser_init(parser_t *p, char *buf, size_t size)
{
    lex_init(&p->l, buf, size);
    p->next_token = NULL;
    p->next_token2 = NULL;
    p->scope = NULL;
    p->func_scope = scope_new(NULL);
    p->frame_offset = 0;
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
        parser_next(p); // consume it
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
        fprintf(stderr, "Expected %s but received '%.*s'\n",
                token_type_to_str(type, str, 100), t->len, t->sval);
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

node_t *parser_type_specifier(parser_t *p)
{
    token_t *tok = parser_next(p);
    node_t *node = node_create(ND_TYPE_SPEC, tok->line, tok->col);
    if (tok->type == TOKEN_KW_VOID) {
        node->type_spec = ND_TYPE_VOID;
    } else if (tok->type == TOKEN_KW_CHAR) {
        node->type_spec = ND_TYPE_CHAR;
    } else if (tok->type == TOKEN_KW_INT) {
        node->type_spec = ND_TYPE_INT;
    } else {
        token_print_error(tok, "type");
        node_destroy(node);
        node = NULL;
    }
    token_destroy(tok);
    return node;
}

node_t *parser_declaration_specifiers(parser_t *p)
{
    node_t *type_spec = parser_type_specifier(p);
    if (type_spec == NULL) {
        return NULL;
    }
    node_t *node = node_create(ND_DECL_SPEC, type_spec->line, type_spec->col);
    node->decl_spec.type_spec = type_spec;
    return node;
}

node_t *parser_parameter_declaration(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL) {
        return NULL;
    }
    node_t *declarator = parser_declarator(p);
    if (declarator == NULL) {
        node_destroy(decl_spec);
        return NULL;
    }
    node_t *n = node_create(ND_PARAM_DECL, decl_spec->line, decl_spec->col);
    n->param_decl.decl_spec = decl_spec;
    n->param_decl.declarator = declarator;
    return n;
}

node_t *parser_parameter_list(parser_t *p)
{
    node_t *param_decl = parser_parameter_declaration(p);
    if (param_decl == NULL)
        return NULL;

    node_t *n = node_create(ND_PARAM_LIST, param_decl->line, param_decl->col);
    n->param_list.params[0] = param_decl;

    int nparams = 1;
    while (parser_accept(p, ',')) {
        param_decl = parser_parameter_declaration(p);
        if (param_decl == NULL)
            return NULL;
        n->param_list.params[nparams++] = param_decl;
    }
    n->param_list.nparams = nparams;
    return n;
}

node_t *parser_direct_declarator(parser_t *p)
{
    // Identifier
    token_t *ident = parser_expect_token(p, TOKEN_IDENT);
    if (ident == NULL) {
        return NULL;
    }

    node_t *n = node_create(ND_DIRECT_DECL, ident->line, ident->col);
    n->direct_decl.ident.str = ident->sval;
    n->direct_decl.ident.len = ident->len;
    n->direct_decl.param_list = NULL;

    // Declarator
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
    return n;
}

int parser_pointer(parser_t *p)
{
    int pointer_level = 0;
    if (!parser_expect(p, '*'))
        return pointer_level;
    ++pointer_level;
    while (1) {
        if (!parser_accept(p, '*'))
            return pointer_level;
        ++pointer_level;
    }
    return pointer_level;
}

node_t *parser_declarator(parser_t *p)
{
    int pointer_level = 0;
    if (parser_peek(p)->type == '*') {
        pointer_level = parser_pointer(p);
        if (pointer_level <= 0) {
            return NULL;
        }
    }
    node_t *n = parser_direct_declarator(p);
    if (n == NULL)
        return NULL;
    n->direct_decl.pointer_level = pointer_level;
    return n;
}

static bool parser_is_type_token(token_type_t type)
{
    return type == TOKEN_KW_INT || type == TOKEN_KW_CHAR ||
           type == TOKEN_KW_VOID;
}

/* Return the byte size of a type given its decl_spec and pointer level */
static int parser_sym_size(node_t *decl_spec, int pointer_level)
{
    if (pointer_level > 0)
        return 8;
    if (decl_spec == NULL || decl_spec->decl_spec.type_spec == NULL)
        return 4;
    switch (decl_spec->decl_spec.type_spec->type_spec) {
        case ND_TYPE_CHAR:
            return 1;
        case ND_TYPE_INT:
            return 4;
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

    int pointer_level = declarator->direct_decl.pointer_level;
    int size = parser_sym_size(decl_spec, pointer_level);
    p->frame_offset =
        parser_align_down(p->frame_offset - size, size < 8 ? size : 8);

    sym_t *sym = scope_define(p->scope, declarator->direct_decl.ident.str,
                              declarator->direct_decl.ident.len, decl_spec,
                              pointer_level, p->frame_offset);

    node_t *init = NULL;
    if (parser_accept(p, '=')) {
        init = parser_assignment_expression(p);
        if (init == NULL) {
            node_destroy(decl_spec);
            node_destroy(declarator);
            return NULL;
        }
    }

    if (!parser_expect(p, ';')) {
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

// Parse a type name for cast / sizeof: declaration_specifiers + optional '*'*
static node_t *parser_type_name(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL)
        return NULL;
    int pointer_level = 0;
    while (parser_accept(p, '*'))
        pointer_level++;
    decl_spec->decl_spec.pointer_level = pointer_level;
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
node_t *parser_primary_expression(parser_t *p)
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
                n->ident.sym =
                    scope_lookup(p->func_scope, tok->sval, tok->len);
            if (n->ident.sym == NULL)
                fprintf(stderr,
                        "%d:%d: warning: undeclared identifier '%.*s'\n",
                        tok->line, tok->col, tok->len, tok->sval);
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
        parser_next(p); // consume '('
        node_t *inner = parser_expression(p);
        if (inner == NULL)
            return NULL;
        if (!parser_expect(p, ')')) {
            node_destroy(inner);
            return NULL;
        }
        return inner;
    }
    fprintf(stderr, "Expected expression but received '%.*s'\n", tok->len,
            tok->sval);
    return NULL;
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
node_t *parser_postfix_expression(parser_t *p)
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
            token_destroy(field);
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
            token_destroy(field);
            node = mem;

        } else if (peek->type == TOKEN_INC_OP || peek->type == TOKEN_DEC_OP) {
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
node_t *parser_unary_expression(parser_t *p)
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
        node_t *n = node_create(ND_UNOP, op_tok->line, op_tok->col);
        n->unop.op = op_tok->type;
        n->unop.operand = operand;
        token_destroy(op_tok);
        return n;
    }

    if (peek->type == '&' || peek->type == '*' || peek->type == '+' ||
        peek->type == '-' || peek->type == '~' || peek->type == '!') {
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
            if (p2 != NULL && parser_is_type_token(p2->type)) {
                parser_next(p); // consume '('
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
node_t *parser_cast_expression(parser_t *p)
{
    token_t *p1 = parser_peek(p);
    if (p1 != NULL && p1->type == '(') {
        token_t *p2 = parser_peek2(p);
        if (p2 != NULL && parser_is_type_token(p2->type)) {
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
node_t *parser_multiplicative_expression(parser_t *p)
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
node_t *parser_additive_expression(parser_t *p)
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
node_t *parser_shift_expression(parser_t *p)
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
node_t *parser_relational_expression(parser_t *p)
{
    node_t *node = parser_shift_expression(p);
    if (node == NULL)
        return NULL;
    while (1) {
        token_t *peek = parser_peek(p);
        if (peek == NULL)
            break;
        if (peek->type != '<' && peek->type != '>' &&
            peek->type != TOKEN_LE_OP && peek->type != TOKEN_GE_OP)
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
node_t *parser_equality_expression(parser_t *p)
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
node_t *parser_and_expression(parser_t *p)
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
node_t *parser_exclusive_or_expression(parser_t *p)
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
node_t *parser_inclusive_or_expression(parser_t *p)
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
node_t *parser_logical_and_expression(parser_t *p)
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
node_t *parser_logical_or_expression(parser_t *p)
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
node_t *parser_conditional_expression(parser_t *p)
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
    return type == '=' || type == TOKEN_MUL_ASSIGN ||
           type == TOKEN_DIV_ASSIGN || type == TOKEN_MOD_ASSIGN ||
           type == TOKEN_ADD_ASSIGN || type == TOKEN_SUB_ASSIGN ||
           type == TOKEN_LEFT_ASSIGN || type == TOKEN_RIGHT_ASSIGN ||
           type == TOKEN_AND_ASSIGN || type == TOKEN_XOR_ASSIGN ||
           type == TOKEN_OR_ASSIGN;
}

/*
 * assignment_operator: '=' | MUL_ASSIGN | ... | OR_ASSIGN
 * assignment_expression
 * : conditional_expression
 * | unary_expression assignment_operator assignment_expression
 * ;
 */
node_t *parser_assignment_expression(parser_t *p)
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
node_t *parser_expression(parser_t *p)
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

node_t *parser_expression_statement(parser_t *p)
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
node_t *parser_selection_statement(parser_t *p)
{
    token_t *t = parser_next(p);
    if (t->type == TOKEN_KW_IF) {
        if (!parser_expect(p, '('))
            return NULL;
        node_t *cond = parser_expression(p);
        if (!cond)
            return NULL;
        if (!parser_expect(p, ')'))
            return NULL;

        node_t *then = parser_statement(p);
        if (!then)
            return NULL;

        node_t *else_ = NULL;
        if (parser_peek(p)->type == TOKEN_KW_ELSE) {
            parser_next(p); // consume 'else'
            else_ = parser_statement(p);
            if (!else_)
                return NULL;
        }

        node_t *n = node_create(ND_IF_STMT, t->line, t->col);
        n->if_stmt.cond = cond;
        n->if_stmt.then = then;
        n->if_stmt.else_ = else_;
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
node_t *parser_iteration_statement(parser_t *p)
{
    token_t *t = parser_next(p); // consume 'while'
    if (t->type == TOKEN_KW_WHILE) {
        if (!parser_expect(p, '('))
            return NULL;
        node_t *cond = parser_expression(p);
        if (!cond)
            return NULL;
        if (!parser_expect(p, ')'))
            return NULL;

        node_t *body = parser_statement(p);
        if (!body)
            return NULL;

        node_t *n = node_create(ND_WHILE_STMT, t->line, t->col);
        n->while_stmt.cond = cond;
        n->while_stmt.body = body;
        return n;
    }
    return NULL;
}

node_t *parser_jump_statement(parser_t *p)
{
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

node_t *parser_statement(parser_t *p)
{
    token_t *t = parser_peek(p);
    if (t->type == '{') {
        return parser_compound_statement(p);
    } else if (t->type == TOKEN_KW_IF) {
        return parser_selection_statement(p);
    } else if (t->type == TOKEN_KW_WHILE) {
        return parser_iteration_statement(p);
    } else if (t->type == TOKEN_KW_RETURN) {
        return parser_jump_statement(p);
    } else {
        return parser_expression_statement(p);
    }
}

static bool parser_block_item_list(parser_t *p, node_t *comp)
{
    while (parser_peek(p)->type != '}') {
        node_t *item;
        if (parser_is_type_token(parser_peek(p)->type))
            item = parser_local_declaration(p);
        else
            item = parser_statement(p);
        if (item == NULL)
            return false;
        comp->comp_stmt.stmts[comp->comp_stmt.nstmts++] = item;
    }
    return true;
}

node_t *parser_compound_statement(parser_t *p)
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

node_t *parser_function_definition(parser_t *p)
{
    node_t *decl_spec = parser_declaration_specifiers(p);
    if (decl_spec == NULL) {
        return NULL;
    }
    node_t *declarator = parser_declarator(p);
    if (declarator == NULL) {
        node_destroy(decl_spec);
        return NULL;
    }

    // Set up symbol table scope for the function body
    p->frame_offset = 0;
    p->scope = scope_new(NULL);

    // Register parameters in the function scope
    node_t *param_list = declarator->direct_decl.param_list;
    if (param_list != NULL) {
        for (int i = 0; i < param_list->param_list.nparams; i++) {
            node_t *pd = param_list->param_list.params[i];
            int ptr_lvl = pd->param_decl.declarator->direct_decl.pointer_level;
            int size = parser_sym_size(pd->param_decl.decl_spec, ptr_lvl);
            p->frame_offset =
                parser_align_down(p->frame_offset - size, size < 8 ? size : 8);
            scope_define(p->scope,
                         pd->param_decl.declarator->direct_decl.ident.str,
                         pd->param_decl.declarator->direct_decl.ident.len,
                         pd->param_decl.decl_spec, ptr_lvl, p->frame_offset);
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
    scope_define(p->func_scope, fname.str, fname.len,
                 decl_spec, declarator->direct_decl.pointer_level, 0);

    return node;
}

node_t *parser_translation_unit(parser_t *p)
{
    node_t *tu = node_create(ND_TRANSLATION_UNIT, 0, 0);
    while (parser_peek(p) != NULL) {
        node_t *func = parser_function_definition(p);
        if (func == NULL) {
            node_destroy(tu);
            return NULL;
        }
        tu->translation_unit.funcs[tu->translation_unit.nfuncs++] = func;
    }
    if (tu->translation_unit.nfuncs == 0) {
        node_destroy(tu);
        return NULL;
    }
    return tu;
}

node_t *parser_exec(parser_t *p)
{
    return parser_translation_unit(p);
}
