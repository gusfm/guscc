#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_IDENT = 0x100, // identifier
    TOKEN_NUM,           // constant number
    TOKEN_STR,           // string literal
    TOKEN_KW_CHAR,       // char keyword
    TOKEN_KW_IF,         // if keyword
    TOKEN_KW_INT,        // int keyword
    TOKEN_KW_RETURN,     // return keyword
    TOKEN_KW_VOID,       // void keyword
    TOKEN_KW_WHILE,      // while keyword
    TOKEN_KW_SIZEOF,     // sizeof keyword
    TOKEN_INC_OP,        // ++
    TOKEN_DEC_OP,        // --
    TOKEN_PTR_OP,        // ->
    TOKEN_LEFT_OP,       // <<
    TOKEN_RIGHT_OP,      // >>
    TOKEN_LE_OP,         // <=
    TOKEN_GE_OP,         // >=
    TOKEN_EQ_OP,         // ==
    TOKEN_NE_OP,         // !=
    TOKEN_AND_OP,        // &&
    TOKEN_OR_OP,         // ||
    TOKEN_MUL_ASSIGN,    // *=
    TOKEN_DIV_ASSIGN,    // /=
    TOKEN_MOD_ASSIGN,    // %=
    TOKEN_ADD_ASSIGN,    // +=
    TOKEN_SUB_ASSIGN,    // -=
    TOKEN_LEFT_ASSIGN,   // <<=
    TOKEN_RIGHT_ASSIGN,  // >>=
    TOKEN_AND_ASSIGN,    // &=
    TOKEN_XOR_ASSIGN,    // ^=
    TOKEN_OR_ASSIGN,     // |=
    TOKEN_EOF
} token_type_t;

typedef struct {
    token_type_t type; // Token type
    char *sval;        // String literal (not null terminated)
    int len;           // Token length
    int line;          // Token line
    int col;           // Token column
} token_t;

token_t *token_create(token_type_t type, char *start, char *end, int line,
                      int col);
void token_destroy(token_t *t);
void token_print(token_t *t);
const char *token_type_to_str(token_type_t type, char *str, size_t len);
void token_print_error(token_t *t, const char *expected);

#endif /* TOKEN_H */
