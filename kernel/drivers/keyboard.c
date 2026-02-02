/* PS/2 keyboard: scancode -> ASCII ring buffer */
#include <stdint.h>
#include <stdbool.h>
#include "kernel/drivers/pic.h"

#define KB_DATA    0x60
#define KB_STATUS  0x64
#define BUF_SIZE   256

static char keybuf[BUF_SIZE];
static volatile uint32_t keybuf_head, keybuf_tail;

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

static void kb_wait_input_clear(void)
{
    while (inb(KB_STATUS) & 0x02) { }
}

static void kb_wait_output_full(void)
{
    while (!(inb(KB_STATUS) & 0x01)) { }
}

/* US QWERTY scancode set 1 -> ASCII (make only, no shift) */
static const char scancode_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};
#define SCANCODE_MAX (sizeof(scancode_ascii) / sizeof(scancode_ascii[0]))

void keyboard_init(void)
{
    keybuf_head = keybuf_tail = 0;

    /* Enable PS/2 keyboard and IRQ1 in controller config */
    kb_wait_input_clear();
    outb(KB_STATUS, 0xAE); /* enable keyboard */

    kb_wait_input_clear();
    outb(KB_STATUS, 0x20); /* read config byte */
    kb_wait_output_full();
    uint8_t cfg = inb(KB_DATA);
    cfg |= 0x01;   /* enable IRQ1 */
    cfg |= 0x40;   /* enable scancode translation (set 2 -> set 1) */
    cfg &= ~0x10;  /* ensure keyboard not disabled */
    kb_wait_input_clear();
    outb(KB_STATUS, 0x60); /* write config byte */
    kb_wait_input_clear();
    outb(KB_DATA, cfg);

    /* Flush any pending data */
    while (inb(KB_STATUS) & 0x01) {
        (void)inb(KB_DATA);
    }
}

static void handle_scancode(uint8_t sc)
{
    if (sc & 0x80) return;   /* break code */
    if (sc < SCANCODE_MAX) {
        char c = scancode_ascii[sc];
        if (c) {
            uint32_t next = (keybuf_head + 1) % BUF_SIZE;
            if (next != keybuf_tail) {
                keybuf[keybuf_head] = c;
                keybuf_head = next;
            }
        }
    }
}

void irq_keyboard_handler(void)
{
    uint8_t sc = inb(KB_DATA);  /* Read scancode first */
    handle_scancode(sc);
    pic_eoi(1);                  /* Send EOI after processing */
}

bool keyboard_getchar(char *c)
{
    if (keybuf_tail == keybuf_head) {
        /* Fallback polling if IRQs are not firing */
        if (inb(KB_STATUS) & 0x01) {
            uint8_t sc = inb(KB_DATA);
            handle_scancode(sc);
        }
        if (keybuf_tail == keybuf_head) return false;
    }
    *c = keybuf[keybuf_tail];
    keybuf_tail = (keybuf_tail + 1) % BUF_SIZE;
    return true;
}
