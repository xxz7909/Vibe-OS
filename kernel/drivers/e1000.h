#ifndef E1000_H
#define E1000_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void e1000_init(void);
bool e1000_send(const void *buf, size_t len);
int e1000_recv(void *buf, size_t max_len);

#endif
