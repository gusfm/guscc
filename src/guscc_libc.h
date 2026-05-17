#ifndef GUSCC_LIBC_H
#define GUSCC_LIBC_H

/*
 * libc shim for self-hosting.
 *
 * When the compiler is built by host gcc (or another stage-1 compiler),
 * this file pulls in the standard system headers. When the compiler is
 * built by guscc itself (__GUSCC__ is defined — see guscc's preprocessor
 * invocation), it instead forward-declares the libc subset the compiler
 * actually uses. This avoids parsing GCC-decorated system headers
 * (__attribute__, __restrict, __nothrow__, __builtin_*, long double, etc.).
 */

#ifdef __GUSCC__

typedef unsigned long size_t;
typedef void FILE; /* opaque — we never dereference, only pass pointers */

extern FILE *stderr;

#define NULL ((void *)0)
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* stdio.h */
int printf(const char *fmt, ...);
int fprintf(FILE *stream, const char *fmt, ...);
int snprintf(char *s, size_t n, const char *fmt, ...);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
FILE *popen(const char *cmd, const char *type);
int pclose(FILE *stream);
void perror(const char *s);

/* stdlib.h */
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
int system(const char *cmd);

/* string.h */
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
char *strncat(char *dest, const char *src, size_t n);
char *strncpy(char *dest, const char *src, size_t n);

/* ctype.h */
int isspace(int c);
int isdigit(int c);
int isalnum(int c);

/* unistd.h */
int close(int fd);
int unlink(const char *path);
int mkstemp(char *template);

#else

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#endif

#endif /* GUSCC_LIBC_H */
