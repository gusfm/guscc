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
    TOKEN_KW_INT,
    TOKEN_KW_RETURN,
    TOKEN_KW_VOID,
    TOKEN_EOF
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
