#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>

void serial_init(void);
void serial_puts(const char *s);
void serial_putc(char c);
bool serial_getchar(char *c);

#endif
