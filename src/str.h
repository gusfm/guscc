#ifndef STR_H
#define STR_H

typedef struct {
    char *str;
    int buf_size;
    int str_len;
} str_t;

str_t *str_create(void);
char *str_destroy(str_t *s);
void str_append(str_t *s, char c);

#endif /* STR_H */
