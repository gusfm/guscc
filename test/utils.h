#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#if defined(__STRICT_ANSI__)
FILE *fmemopen(void *buf, size_t size, const char *mode);
#endif

#endif /* UTILS_H */
