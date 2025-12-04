#ifndef AST_H
#define AST_H

#include "token.h"

typedef enum {
    ND_FUNC,
    ND_DECL_SPEC,
    ND_TYPE_SPEC,
    ND_PARAM_DECL,
    ND_PARAM_LIST,
    ND_DIRECT_DECL,
} node_kind_t;

typedef struct node node_t;

typedef struct {
    char *str;
    int len;
} node_str_t;

struct node {
    node_kind_t kind;
    int line;
    int col;
    union {
        enum {
            ND_TYPE_VOID,
            ND_TYPE_CHAR,
            ND_TYPE_INT,
        } type_spec;

        struct {
            node_t *type_spec;
        } decl_spec;

        struct {
            node_t *decl_spec;
            node_t *declarator;
            node_t *comp_stmt;
        } func;

        struct {
            node_t *decl_spec;
            node_t *declarator;
        } param_decl;

        struct {
            int nparams;
            node_t *params[8]; // MAX_PARAM == 8
        } param_list;

        struct {
            node_str_t ident;
            node_t *param_list;
        } direct_decl;
    };
};

/*
typedef struct {
    node_t *nodes;
    size_t used;
    size_t capacity;
} ast_t;
*/
node_t *node_create(node_kind_t kind, int line, int col);
void node_destroy(node_t *node);

void ast_print(node_t *n, int indent);

#endif /* AST_H */
