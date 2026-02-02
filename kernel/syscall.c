/* Syscall handler: RAX=number, RDI, RSI, RDX = args. Use sysexit to return. */
#include <stdint.h>
#include <stddef.h>
#include "kernel/syscall.h"
#include "kernel/drivers/vga.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/drivers/serial.h"
#include "kernel/task/sched.h"
#include "kernel/drivers/e1000.h"

extern void syscall_entry(void);

/* MSR addresses for syscall/sysret */
#define MSR_EFER   0xC0000080
#define MSR_STAR   0xC0000081
#define MSR_LSTAR  0xC0000082
#define MSR_SFMASK 0xC0000084

/* GDT selectors: kernel code=0x08, kernel data=0x10, user data=0x18, user code=0x20 */
#define KERNEL_CS  0x08
#define SYSRET_BASE 0x10   /* SYSRET: CS = base+16 = 0x20, SS = base+8 = 0x18 */

void syscall_init(void)
{
    uint32_t eax, edx;

    /* Enable SCE (syscall enable) in EFER */
    __asm__ volatile ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(MSR_EFER));
    eax |= 1;
    __asm__ volatile ("wrmsr" : : "a"(eax), "d"(edx), "c"(MSR_EFER));

    /* STAR: bits 32-47 = SYSCALL CS (0x08), bits 48-63 = SYSRET base (0x10) */
    uint64_t star = ((uint64_t)SYSRET_BASE << 48) | ((uint64_t)KERNEL_CS << 32);
    __asm__ volatile ("wrmsr" : : "a"((uint32_t)star), "d"((uint32_t)(star >> 32)), "c"(MSR_STAR));

    /* LSTAR: syscall entry point */
    uint64_t addr = (uint64_t)syscall_entry;
    __asm__ volatile ("wrmsr" : : "a"((uint32_t)addr), "d"((uint32_t)(addr >> 32)), "c"(MSR_LSTAR));

    /* SFMASK: clear IF (0x200) on syscall to disable interrupts */
    __asm__ volatile ("wrmsr" : : "a"(0x200), "d"(0), "c"(MSR_SFMASK));
}

/* frame layout: [0]=rdx, [1]=rsi, [2]=rdi, [3]=rax */
void syscall_handler(void *frame_ptr)
{
    uint64_t *f = (uint64_t *)frame_ptr;
    uint64_t nr = f[3], a1 = f[2], a2 = f[1], a3 = f[0];
    switch (nr) {
    case SYS_WRITE: {
        int fd = (int)a1;
        const char *buf = (const char *)a2;
        size_t count = (size_t)a3;
        (void)fd;
        for (size_t i = 0; i < count && buf[i]; i++) {
            char tmp[2] = { buf[i], '\0' };
            vga_puts(tmp);
        }
        break;
    }
    case SYS_READ: {
        int fd = (int)a1;
        char *buf = (char *)a2;
        size_t count = (size_t)a3;
        (void)fd;
        size_t n = 0;
        while (n < count) {
            char ch;
            /* Enable interrupts while waiting for input */
            __asm__ volatile ("sti");
            while (!keyboard_getchar(&ch)) {
                if (serial_getchar(&ch)) break;
                __asm__ volatile ("hlt");  /* Wait for interrupt */
            }
            buf[n++] = ch;
            /* Echo input so user can see what they type */
            char tmp[2] = { ch, '\0' };
            vga_puts(tmp);
            serial_putc(ch);
            if (ch == '\n' || ch == '\r') {
                vga_puts("\r\n");
                serial_puts("\r\n");
                break;
            }
        }
        f[3] = n;
        break;
    }
    case SYS_EXIT:
        sched_schedule();
        break;
    case SYS_NET_SEND: {
        const void *p = (const void *)a1;
        size_t len = (size_t)a2;
        f[3] = e1000_send(p, len) ? 0 : -1;
        break;
    }
    case SYS_NET_RECV: {
        void *p = (void *)a1;
        size_t max_len = (size_t)a2;
        int n = e1000_recv(p, max_len);
        f[3] = (uint64_t)(n < 0 ? -1 : n);
        break;
    }
    default:
        vga_puts("\r\n[unknown syscall]\r\n");
        break;
    }
}
