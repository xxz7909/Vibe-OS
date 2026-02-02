#ifndef SCHED_H
#define SCHED_H
#include <stdint.h>
#include "kernel/task/pcb.h"

extern uint8_t next_is_user;

void sched_init(void);
void sched_add(pcb_t *p);
void sched_schedule(void);
pcb_t *sched_current(void);

#endif
