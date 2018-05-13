#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_IDENT = 0x100,
    TOKEN_CHAR,
    TOKEN_STRING_LITERAL,
    TOKEN_ELLIPSIS,
    TOKEN_RSH_ASSIGN,
    TOKEN_LSH_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_RSH_OP,
    TOKEN_LSH_OP,
    TOKEN_INC_OP,
    TOKEN_DEC_OP,
    TOKEN_PTR_OP,
    TOKEN_AND_OP,
    TOKEN_OR_OP,
    TOKEN_LE_OP,
    TOKEN_GE_OP,
    TOKEN_EQ_OP,
    TOKEN_NE_OP,
    TOKEN_NUMBER,
    TOKEN_KW_AUTO,
    TOKEN_KW_BREAK,
    TOKEN_KW_CASE,
    TOKEN_KW_CHAR,
    TOKEN_KW_CONST,
    TOKEN_KW_CONTINUE,
    TOKEN_KW_DEFAULT,
    TOKEN_KW_DO,
    TOKEN_KW_DOUBLE,
    TOKEN_KW_ELSE,
    TOKEN_KW_ENUM,
    TOKEN_KW_EXTERN,
    TOKEN_KW_FLOAT,
    TOKEN_KW_FOR,
    TOKEN_KW_GOTO,
    TOKEN_KW_IF,
    TOKEN_KW_INT,
    TOKEN_KW_LONG,
    TOKEN_KW_REGISTER,
    TOKEN_KW_RETURN,
    TOKEN_KW_SHORT,
    TOKEN_KW_SIGNED,
    TOKEN_KW_SIZEOF,
    TOKEN_KW_STATIC,
    TOKEN_KW_STRUCT,
    TOKEN_KW_SWITCH,
    TOKEN_KW_TYPEDEF,
    TOKEN_KW_UNION,
    TOKEN_KW_UNSIGNED,
    TOKEN_KW_VOID,
    TOKEN_KW_VOLATILE,
    TOKEN_KW_WHILE
} token_type_t;

typedef struct {
    token_type_t type;
    int line;
    int col;
    union {
        char *s;
        char c;
    } val;
} token_t;

token_t *token_create(token_type_t type);
token_t *token_create_char(char c);
token_t *token_create_string(token_type_t type, char *s);
void token_destroy(token_t *t);

#endif /* TOKEN_H */
