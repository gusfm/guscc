#ifndef IDENT_H
#define IDENT_H

typedef struct {
    char *str;
    int buf_size;
    int str_len;
} ident_t;

ident_t *ident_create(void);
char *ident_destroy(ident_t *i);
void ident_append(ident_t *i, char c);

#endif /* IDENT_H */
