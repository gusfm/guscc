#include "ident.h"
#include <stdlib.h>

#define IDENT_INIT_SIZE 8

ident_t *ident_create(void)
{
    ident_t *i = malloc(sizeof(ident_t));
    i->buf_size = IDENT_INIT_SIZE;
    i->str_len = 0;
    i->str = malloc(IDENT_INIT_SIZE);
    return i;
}

char *ident_destroy(ident_t *i)
{
    char *s = i->str;
    free(i);
    return s;
}

void ident_append(ident_t *i, char c)
{
    if (i->str_len + 1 == i->buf_size) {
        i->buf_size *= 2;
        i->str = realloc(i->str, i->buf_size);
    }
    i->str[i->str_len++] = c;
}
