#include <stdint.h>
#include <stdbool.h>

#define COM1 0x3F8

static inline uint8_t inb(uint16_t port)
{
    uint8_t v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static inline void outb(uint16_t port, uint8_t v)
{
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

void serial_init(void)
{
    outb(COM1 + 1, 0);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void serial_putc(char c)
{
    while (!(inb(COM1 + 5) & 0x20));
    outb(COM1, (uint8_t)c);
}

void serial_puts(const char *s)
{
    while (*s) { serial_putc(*s); s++; }
}

bool serial_getchar(char *c)
{
    if (!(inb(COM1 + 5) & 0x01)) return false;
    *c = (char)inb(COM1);
    return true;
}
