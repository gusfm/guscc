#include "str.h"
#include <stdlib.h>

#define STRING_INIT_SIZE 8

str_t *str_create(void)
{
    str_t *i = malloc(sizeof(str_t));
    i->buf_size = STRING_INIT_SIZE;
    i->str_len = 0;
    i->str = malloc(STRING_INIT_SIZE);
    return i;
}

char *str_destroy(str_t *i)
{
    char *s = i->str;
    free(i);
    return s;
}

void str_append(str_t *i, char c)
{
    if (i->str_len + 1 == i->buf_size) {
        i->buf_size *= 2;
        i->str = realloc(i->str, i->buf_size);
    }
    i->str[i->str_len++] = c;
}
