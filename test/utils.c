#include <stdio.h>
#include "utils.h"

#if defined(__STRICT_ANSI__)

FILE *fmemopen(void *buf, size_t size, const char *mode)
{
    size_t written;
    FILE *f = fopen("fmemopen_tmp.c", mode);
    if (f == NULL) {
        perror("fopen");
        return NULL;
    }
    written = fwrite(buf, size, 1, f);
    if (written != 1) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    return f;
}

#endif
