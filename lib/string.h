#ifndef LIB_STRING_H
#define LIB_STRING_H

#include <stddef.h>

size_t strlen(const char *s);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int strcmp(const char *a, const char *b);

#endif
