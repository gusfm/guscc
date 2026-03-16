#ifndef AST_H
#define AST_H

#include "token.h"

typedef enum {
    ND_FUNC,        // Function definition
    ND_DECL_SPEC,   // Declaration specifiers (e.g. int, char)
    ND_TYPE_SPEC,   // Type specifier leaf (void / char / int)
    ND_PARAM_DECL,  // Single parameter declaration
    ND_PARAM_LIST,  // Comma-separated parameter list
    ND_DIRECT_DECL, // Declarator: identifier + optional ptr '*' + param list
    ND_COMP_STMT,   // Compound statement  { ... }
    ND_RETURN_STMT, // return [expr] ;
    ND_EXPR_STMT,   // [expr] ;
    ND_NUM,         // Number literal
} node_kind_t;

typedef struct node node_t;

typedef struct {
    char *str; // pointer into the source buffer (not null-terminated)
    int len;   // length of the string
} node_str_t;

struct node {
    node_kind_t kind; // discriminant for the union below
    int line;         // source line (1-based)
    int col;          // source column (1-based)
    union {
        enum {
            ND_TYPE_VOID, // void
            ND_TYPE_CHAR, // char
            ND_TYPE_INT,  // int
        } type_spec;      // used when kind == ND_TYPE_SPEC

        struct {
            node_t *type_spec; // the type specifier node (e.g. int, char)
        } decl_spec;           // used when kind == ND_DECL_SPEC

        struct {
            node_t *decl_spec;  // return type
            node_t *declarator; // name, pointer level, and parameter list
            node_t *comp_stmt;  // function body
        } func;                 // used when kind == ND_FUNC

        struct {
            node_t *decl_spec;  // type of the parameter
            node_t *declarator; // name and pointer level of the parameter
        } param_decl;           // used when kind == ND_PARAM_DECL

        struct {
            int nparams;       // number of entries in params[]
            node_t *params[8]; // MAX_PARAM == 8
        } param_list;          // used when kind == ND_PARAM_LIST

        struct {
            node_str_t ident;   // function or variable name
            int pointer_level;  // number of leading '*' stars
            node_t *param_list; // NULL for non-function declarators
        } direct_decl;          // used when kind == ND_DIRECT_DECL

        struct {
            int nstmts;        // number of entries in stmts[]
            node_t *stmts[64]; // MAX_STMTS == 64
        } comp_stmt;           // used when kind == ND_COMP_STMT

        struct {
            node_t *expr; // NULL for bare return;
        } return_stmt;    // used when kind == ND_RETURN_STMT

        struct {
            node_t *expr; // NULL for empty statement ';'
        } expr_stmt;      // used when kind == ND_EXPR_STMT

        struct {
            node_str_t val; // raw token string (not null-terminated)
        } num;              // used when kind == ND_NUM
    };
};

node_t *node_create(node_kind_t kind, int line, int col);
void node_destroy(node_t *node);

void ast_print(node_t *n, int indent);

#endif /* AST_H */
