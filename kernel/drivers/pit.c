/* PIT: channel 0, ~100 Hz for scheduler tick */
#include <stdint.h>
#include "kernel/drivers/pic.h"
#include "kernel/task/sched.h"

#define PIT_FREQ    1193182
#define TARGET_HZ   100
#define PIT_CH0     0x40
#define PIT_CMD     0x43

static inline void outb(uint16_t port, uint8_t v)
{
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

void pit_init(void)
{
    uint32_t divisor = PIT_FREQ / TARGET_HZ;
    outb(PIT_CMD, 0x34);   /* ch0, lo/hi, rate generator */
    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);
}

static volatile uint64_t ticks;

void irq_pit_handler(void)
{
    pic_eoi(0);
    ticks++;
    sched_schedule();
}

uint64_t pit_ticks(void) { return ticks; }
