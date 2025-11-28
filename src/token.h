#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_IDENT = 0x100,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_KW_CHAR,
    TOKEN_KW_IF,
    TOKEN_KW_INT,
    TOKEN_KW_RETURN,
    TOKEN_KW_VOID,
    TOKEN_KW_WHILE,
    TOKEN_EOF
} token_type_t;

typedef struct {
    token_type_t type; // Token type
    char *sval;        // String literal (not null terminated)
    int len;           // Token length
} token_t;

token_t *token_create(token_type_t type, char *start, char *end);
void token_destroy(token_t *t);
void token_print(token_t *t);
const char *token_type_to_str(token_type_t type, char *str, size_t len);

#endif /* TOKEN_H */
