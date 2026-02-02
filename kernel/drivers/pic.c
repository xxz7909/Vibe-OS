/* PIC 8259: remap IRQ 0-15 to int 32-47 and mask all except PIT(0) and keyboard(1) */
#include <stdint.h>

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10
#define ICW4_8086 0x01
#define EOI       0x20

static inline void outb(uint16_t port, uint8_t v)
{
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

void pic_init(void)
{
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, 32);   /* IRQ 0 -> int 32 */
    outb(PIC2_DATA, 40);   /* IRQ 8 -> int 40 */
    outb(PIC1_DATA, 4);    /* slave on line 2 */
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    outb(PIC1_DATA, 0xFC); /* mask: allow IRQ 0 (PIT), IRQ 1 (keyboard) */
    outb(PIC2_DATA, 0xFF);

}

void pic_eoi(int irq)
{
    if (irq >= 8) outb(PIC2_CMD, EOI);
    outb(PIC1_CMD, EOI);
}
