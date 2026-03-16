#ifndef LEX_H
#define LEX_H

#include "token.h"

typedef struct {
    char *start; // Start of file loaded to string
    char *end;   // End of file loaded to string
    char *p;     // Pointer to current file position
    int line;    // Line number of file
    int col;     // Column number of file
} lex_t;

void lex_init(lex_t *l, char *start, size_t size);
token_t *lex_next(lex_t *l);

#endif /* LEX_H */
