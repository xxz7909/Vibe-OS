/* OS-dev kernel: init PMM, VMM, IDT, PIC, PIT, keyboard, sched, syscall; run shell */
#include <stdint.h>
#include <stddef.h>
#include "kernel/drivers/vga.h"
#include "kernel/task/pcb.h"
#include "kernel/drivers/serial.h"
#include "kernel/mem/pmm.h"
#include "kernel/mem/vmm.h"
#include "kernel/idt.h"
#include "kernel/drivers/pic.h"
#include "kernel/drivers/pit.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/task/sched.h"
#include "kernel/syscall.h"
#include "kernel/drivers/ide.h"
#include "kernel/drivers/e1000.h"
#include "kernel/fs/fs.h"
#include "lib/string.h"

extern char _binary_shell_bin_start[], _binary_shell_bin_end[];

/* Simple string compare */
static int str_match(const char *buf, const char *cmd)
{
    while (*cmd) {
        if (*buf != *cmd) return 0;
        buf++; cmd++;
    }
    return *buf == '\0' || *buf == '\n' || *buf == '\r' || *buf == ' ';
}

/* Kernel-mode REPL */
static void kernel_repl(void)
{
    char buf[64];
    int pos = 0;
    
    vga_puts("vibe> ");
    serial_puts("vibe> ");
    
    for (;;) {
        char ch = 0;
        
        /* Try keyboard first, then serial */
        if (!keyboard_getchar(&ch)) {
            if (!serial_getchar(&ch)) {
                __asm__ volatile ("hlt");
                continue;
            }
        }
        
        /* Handle backspace */
        if (ch == '\b' || ch == 127) {
            if (pos > 0) {
                pos--;
                vga_puts("\b \b");
                serial_puts("\b \b");
            }
            continue;
        }
        
        /* Handle enter */
        if (ch == '\n' || ch == '\r') {
            vga_puts("\r\n");
            serial_puts("\r\n");
            buf[pos] = '\0';
            
            /* Process command */
            if (str_match(buf, "hello")) {
                vga_puts("Hello vibe-OS!\r\n");
                serial_puts("Hello vibe-OS!\r\n");
            } else if (str_match(buf, "help")) {
                vga_puts("Commands: hello, help, clear, tick\r\n");
                serial_puts("Commands: hello, help, clear, tick\r\n");
            } else if (str_match(buf, "clear")) {
                vga_clear();
            } else if (str_match(buf, "tick")) {
                char tmp[32];
                uint64_t t = pit_ticks();
                vga_puts("Ticks: ");
                serial_puts("Ticks: ");
                /* Simple number print */
                int i = 0;
                if (t == 0) tmp[i++] = '0';
                else {
                    uint64_t n = t;
                    char rev[20];
                    int j = 0;
                    while (n > 0) { rev[j++] = '0' + (n % 10); n /= 10; }
                    while (j > 0) tmp[i++] = rev[--j];
                }
                tmp[i] = '\0';
                vga_puts(tmp);
                vga_puts("\r\n");
                serial_puts(tmp);
                serial_puts("\r\n");
            } else if (pos > 0) {
                vga_puts("Unknown command. Try 'help'\r\n");
                serial_puts("Unknown command. Try 'help'\r\n");
            }
            
            pos = 0;
            vga_puts("vibe> ");
            serial_puts("vibe> ");
            continue;
        }
        
        /* Echo and store character */
        if (pos < 63 && ch >= 32 && ch < 127) {
            buf[pos++] = ch;
            char tmp[2] = { ch, '\0' };
            vga_puts(tmp);
            serial_putc(ch);
        }
    }
}

void kernel_main(uint32_t magic, uint32_t info_phys)
{
    (void)magic;
    
    vga_clear();
    serial_init();
    vga_puts("Hello vibe-OS\r\n");
    serial_puts("Hello vibe-OS\r\n");

    pmm_init((uintptr_t)info_phys);
    vmm_init();
    pic_init();
    idt_init();
    pit_init();
    keyboard_init();
    
    __asm__ volatile ("sti");  /* Enable interrupts */
    
    vga_puts("REPL ready. Type 'hello' or 'help'.\r\n");
    serial_puts("REPL ready. Type 'hello' or 'help'.\r\n");
    
    kernel_repl();  /* Run kernel-mode REPL */
}
            sched_schedule();
        }
    }
}
