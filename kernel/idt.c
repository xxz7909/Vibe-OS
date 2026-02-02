/* IDT setup and C handler dispatch */
#include <stdint.h>
#include "kernel/drivers/vga.h"
#include "kernel/drivers/serial.h"
#include "kernel/drivers/pit.h"
#include "kernel/drivers/keyboard.h"

#define IDT_ENTRIES 48
#define GATE_INT    0x8E   /* P=1, DPL=0, type=0xE (64-bit interrupt) */

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;

extern void load_idt(void *ptr);

void idt_set_gate(int n, uint64_t handler)
{
    idt[n].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[n].selector   = 8;   /* kernel code segment from GDT */
    idt[n].ist        = 0;
    idt[n].flags      = GATE_INT;
    idt[n].offset_mid = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[n].offset_high= (uint32_t)(handler >> 32);
    idt[n].zero       = 0;
}

void idt_init(void)
{
    extern void isr_0(void), isr_1(void), isr_2(void), isr_3(void), isr_4(void), isr_5(void), isr_6(void), isr_7(void);
    extern void isr_8(void), isr_9(void), isr_10(void), isr_11(void), isr_12(void), isr_13(void), isr_14(void), isr_15(void);
    extern void isr_16(void), isr_17(void), isr_18(void), isr_19(void), isr_20(void), isr_21(void), isr_22(void), isr_23(void);
    extern void isr_24(void), isr_25(void), isr_26(void), isr_27(void), isr_28(void), isr_29(void), isr_30(void), isr_31(void);
    extern void isr_32(void), isr_33(void), isr_34(void), isr_35(void), isr_36(void), isr_37(void), isr_38(void), isr_39(void);
    extern void isr_40(void), isr_41(void), isr_42(void), isr_43(void), isr_44(void), isr_45(void), isr_46(void), isr_47(void);

    void *handlers[] = { isr_0, isr_1, isr_2, isr_3, isr_4, isr_5, isr_6, isr_7,
        isr_8, isr_9, isr_10, isr_11, isr_12, isr_13, isr_14, isr_15,
        isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23,
        isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31,
        isr_32, isr_33, isr_34, isr_35, isr_36, isr_37, isr_38, isr_39,
        isr_40, isr_41, isr_42, isr_43, isr_44, isr_45, isr_46, isr_47 };

    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, (uint64_t)handlers[i]);

    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint64_t)idt;
    load_idt(&idtp);

}

static const char *exception_names[] = {
    "DE", "DB", "NMI", "BP", "OF", "BR", "UD", "NM", "DF", "CSO", "TS", "NP", "SS", "GP", "PF", "??",
    "MF", "AC", "MC", "XM", "VE", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??"
};

void isr_handler(void *frame_ptr)
{
    uint64_t *frame = (uint64_t *)frame_ptr;
    /* Stack: [0-14]=r15..rax, [15]=vector, [16]=error, [17]=rip, ... */
    uint64_t vector = frame[15];
    uint64_t err = frame[16];
    (void)err;

    if (vector < 32) {
        const char *name = vector < 22 ? exception_names[vector] : "??";
        vga_puts("\r\n[EXC ");
        vga_puts(name);
        vga_puts("] ");
        serial_puts("[EXC ");
        serial_puts(name);
        serial_puts(" e=");
        /* Print error code */
        extern void serial_putc(char c);
        for (int i = 12; i >= 0; i -= 4) {
            int n = (err >> i) & 0xF;
            serial_putc(n < 10 ? '0' + n : 'a' + n - 10);
        }
        /* Print CR2 for page faults */
        if (vector == 14) {
            uint64_t cr2;
            __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
            serial_puts(" CR2=");
            for (int i = 28; i >= 0; i -= 4) {
                int n = (cr2 >> i) & 0xF;
                serial_putc(n < 10 ? '0' + n : 'a' + n - 10);
            }
        }
        serial_puts("]\r\n");
        for (;;) __asm__ volatile ("hlt");
    }

    /* IRQ: vector 32 = PIT, 33 = keyboard */
    if (vector == 32) irq_pit_handler();
    else if (vector == 33) irq_keyboard_handler();
}
