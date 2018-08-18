#include "error.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

extern int line;
extern int col;

void ccwarn(const char *format, ...)
{
    va_list args;
    fprintf(stderr, "%d:%d: warning: ", line, col);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void ccerror(const char *format, ...)
{
    va_list args;
    fprintf(stderr, "%d:%d: error: ", line, col);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}
