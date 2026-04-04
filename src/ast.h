#ifndef AST_H
#define AST_H

#include "token.h"

/* Forward declarations — sym.h includes ast.h, so we forward-declare here */
typedef struct sym sym_t;
typedef struct struct_def struct_def_t;
typedef struct struct_member struct_member_t;

typedef enum {
    ND_TRANSLATION_UNIT, // Top-level list of function definitions
    ND_FUNC,             // Function definition
    ND_DECL_SPEC,        // Declaration specifiers (e.g. int, char)
    ND_TYPE_SPEC,        // Type specifier leaf (void / char / int)
    ND_STRUCT_SPEC,      // Struct specifier (tag name + resolved definition)
    ND_ENUM_SPEC,        // Enum specifier (tag name)
    ND_PARAM_DECL,       // Single parameter declaration
    ND_PARAM_LIST,       // Comma-separated parameter list
    ND_DIRECT_DECL,      // Declarator: identifier + optional ptr '*' + param list
    ND_COMP_STMT,        // Compound statement  { ... }
    ND_IF_STMT,          // if (cond) then [else else_]
    ND_SWITCH_STMT,      // switch (expr) body
    ND_CASE_LABEL,       // case expr : stmt
    ND_DEFAULT_LABEL,    // default : stmt
    ND_WHILE_STMT,       // while (cond) body
    ND_DO_WHILE_STMT,    // do body while (cond) ;
    ND_FOR_STMT,         // for ( init ; cond ; post ) body
    ND_BREAK_STMT,       // break ;
    ND_CONTINUE_STMT,    // continue ;
    ND_RETURN_STMT,      // return [expr] ;
    ND_EXPR_STMT,        // [expr] ;
    ND_NUM,              // Number literal
    ND_IDENT,            // Identifier reference
    ND_STR,              // String literal
    ND_BINOP,            // Binary operator (left op right)
    ND_UNOP,             // Prefix unary operator (op operand)
    ND_POSTOP,           // Postfix ++ / --
    ND_SUBSCRIPT,        // a[i]
    ND_CALL,             // f(args...)
    ND_MEMBER,           // a.b or a->b
    ND_CAST,             // (type)expr
    ND_SIZEOF_EXPR,      // sizeof expr
    ND_SIZEOF_TYPE,      // sizeof(type_name)
    ND_TERNARY,          // cond ? then : else
    ND_ASSIGN,           // lvalue op= rvalue
    ND_COMMA,            // expr , expr
    ND_LOCAL_DECL,          // local variable declaration: type name [= init] ;
    ND_GLOBAL_DECL,         // global variable declaration: type name [= init] ;
    ND_INITIALIZER_LIST,    // { initializer, initializer, ... }
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
        struct {
            int nfuncs;
            node_t *funcs[64]; // MAX_FUNCS == 64
            int ndecls;
            node_t *decls[64]; // MAX_DECLS == 64
        } translation_unit;    // used when kind == ND_TRANSLATION_UNIT

        enum {
            ND_TYPE_VOID,  // void
            ND_TYPE_CHAR,  // char
            ND_TYPE_SHORT, // short (2 bytes)
            ND_TYPE_INT,   // int
            ND_TYPE_LONG,  // long (8 bytes)
        } type_spec;       // used when kind == ND_TYPE_SPEC

        struct {
            node_str_t tag;    // struct tag name
            struct_def_t *def; // resolved struct definition (NULL for forward ref)
        } struct_spec;         // used when kind == ND_STRUCT_SPEC

        struct {
            node_str_t tag; // enum tag name (empty for anonymous enums)
        } enum_spec;        // used when kind == ND_ENUM_SPEC

        struct {
            node_t *type_spec; // ND_TYPE_SPEC, ND_STRUCT_SPEC, or ND_ENUM_SPEC
            int pointer_level; // for abstract declarators (cast / sizeof)
        } decl_spec;           // used when kind == ND_DECL_SPEC

        struct {
            node_t *decl_spec;      // return type
            node_t *declarator;     // name, pointer level, and parameter list
            node_t *comp_stmt;      // function body
            int frame_size;         // total stack bytes to reserve (subq $N, %rsp)
            sym_t *params_sym_list; // sym_t chain for params (freed with node)
        } func;                     // used when kind == ND_FUNC

        struct {
            node_t *decl_spec;  // type of the parameter
            node_t *declarator; // name and pointer level of the parameter
        } param_decl;           // used when kind == ND_PARAM_DECL

        struct {
            int nparams;       // number of entries in params[]
            node_t *params[8]; // MAX_PARAM == 8
            int is_variadic;   // 1 if parameter list ends with '...'
        } param_list;          // used when kind == ND_PARAM_LIST

        struct {
            node_str_t ident;   // function or variable name
            int pointer_level;  // number of leading '*' stars
            int array_size;     // 0 = not array, positive = element count, -1 = unsized ([])
            node_t *param_list; // NULL for non-function declarators
        } direct_decl;          // used when kind == ND_DIRECT_DECL

        struct {
            int nstmts;        // number of entries in stmts[]
            node_t *stmts[64]; // MAX_STMTS == 64
        } comp_stmt;           // used when kind == ND_COMP_STMT

        struct {
            node_t *cond;
            node_t *then;
            node_t *else_; // NULL if no else clause
        } if_stmt;         // used when kind == ND_IF_STMT

        struct {
            node_t *expr;
            node_t *body;
        } switch_stmt; // used when kind == ND_SWITCH_STMT

        struct {
            node_t *expr;
            node_t *stmt;
        } case_label; // used when kind == ND_CASE_LABEL

        struct {
            node_t *stmt;
        } default_label; // used when kind == ND_DEFAULT_LABEL

        struct {
            node_t *cond;
            node_t *body;
        } while_stmt; // used when kind == ND_WHILE_STMT and ND_DO_WHILE_STMT

        struct {
            node_t *init; // ND_EXPR_STMT (expr may be NULL for empty init)
            node_t *cond; // expression or NULL (empty cond → always true)
            node_t *post; // expression or NULL (empty post → no increment)
            node_t *body; // loop body statement
        } for_stmt;       // used when kind == ND_FOR_STMT

        struct {
            node_t *expr; // NULL for bare return;
        } return_stmt;    // used when kind == ND_RETURN_STMT

        struct {
            node_t *expr; // NULL for empty statement ';'
        } expr_stmt;      // used when kind == ND_EXPR_STMT

        struct {
            node_str_t val; // raw token string (not null-terminated)
        } num;              // used when kind == ND_NUM

        struct {
            node_str_t name; // identifier name (not null-terminated)
            sym_t *sym;      // resolved symbol; NULL if undeclared
        } ident;             // used when kind == ND_IDENT

        struct {
            node_str_t val; // string value including quotes (not null-terminated)
        } str;              // used when kind == ND_STR

        struct {
            int op; // token type of the operator
            node_t *left;
            node_t *right;
        } binop; // used when kind == ND_BINOP

        struct {
            int op; // token type:
                    // '&','*','+','-','~','!',TOKEN_INC_OP,TOKEN_DEC_OP
            node_t *operand;
        } unop; // used when kind == ND_UNOP

        struct {
            int op; // TOKEN_INC_OP or TOKEN_DEC_OP
            node_t *operand;
        } postop; // used when kind == ND_POSTOP

        struct {
            node_t *array;
            node_t *index;
        } subscript; // used when kind == ND_SUBSCRIPT

        struct {
            node_t *func; // callee expression
            int nargs;
            node_t *args[8]; // MAX_ARGS == 8
        } call;              // used when kind == ND_CALL

        struct {
            node_t *object;             // struct/pointer expression
            node_str_t field;           // member name (not null-terminated)
            int is_ptr;                 // 0 for '.', 1 for '->'
            struct_member_t *resolved;  // resolved member definition; NULL until resolved
        } member;                       // used when kind == ND_MEMBER

        struct {
            node_t *type_node; // ND_DECL_SPEC describing the target type
            node_t *expr;
        } cast; // used when kind == ND_CAST

        struct {
            node_t *expr;
        } sizeof_expr; // used when kind == ND_SIZEOF_EXPR

        struct {
            node_t *type_node; // ND_DECL_SPEC describing the type
        } sizeof_type;         // used when kind == ND_SIZEOF_TYPE

        struct {
            node_t *cond;
            node_t *then_expr;
            node_t *else_expr;
        } ternary; // used when kind == ND_TERNARY

        struct {
            int op; // token type of the assignment operator
            node_t *lhs;
            node_t *rhs;
        } assign; // used when kind == ND_ASSIGN

        struct {
            node_t *left;
            node_t *right;
        } comma; // used when kind == ND_COMMA

        struct {
            node_t *decl_spec;  // ND_DECL_SPEC describing the type
            node_t *declarator; // ND_DIRECT_DECL — name + pointer_level
            node_t *init;       // optional initializer expr; NULL if absent
            sym_t *sym;         // resolved symbol entry
        } local_decl;           // used when kind == ND_LOCAL_DECL

        struct {
            node_t *decl_spec;  // ND_DECL_SPEC describing the type
            node_t *declarator; // ND_DIRECT_DECL — name + pointer_level; NULL for bare type decl
            node_t *init;       // optional initializer expr; NULL if absent
            sym_t *sym;         // resolved symbol (owned by parser global_scope, NOT freed here)
        } global_decl;          // used when kind == ND_GLOBAL_DECL

        struct {
            int count;
            node_t *items[64]; // max 64 initializer elements
        } initializer_list;    // used when kind == ND_INITIALIZER_LIST
    };
};

node_t *node_create(node_kind_t kind, int line, int col);
void node_destroy(node_t *node);

void ast_print(node_t *n, int indent);

#endif /* AST_H */
