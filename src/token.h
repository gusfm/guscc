#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_IDENT,
    TOKEN_OPEN_PAR,
    TOKEN_CLOSE_PAR,
    TOKEN_SEMICOLON,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
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
    char *sval;
} token_t;

token_t *token_create(token_type_t type, int line, int col, char *s);
void token_destroy(token_t *t);
const char *token_type_str(token_type_t type);

#endif /* TOKEN_H */
