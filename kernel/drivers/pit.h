#ifndef PIT_H
#define PIT_H
#include <stdint.h>

void pit_init(void);
void irq_pit_handler(void);
uint64_t pit_ticks(void);

#endif
