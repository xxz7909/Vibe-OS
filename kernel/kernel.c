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

extern char _binary_shell_bin_start[], _binary_shell_bin_end[];

static void kernel_init_early(uintptr_t multiboot_info_phys)
{
    vga_clear();
    serial_init();
    vga_puts("Hello vibe-OS\r\n");
    serial_puts("Hello vibe-OS\r\n");

    pmm_init(multiboot_info_phys);
    vmm_init();
    pic_init();
    idt_init();
    pit_init();
    keyboard_init();
    ide_init();
    e1000_init();
    fs_init();
    if (fs_mount(0, 1))
        vga_puts("FS mounted.\r\n");
    sched_init();
    syscall_init();

    size_t shell_size = (size_t)(_binary_shell_bin_end - _binary_shell_bin_start);
    pcb_t *shell = pcb_create_user(_binary_shell_bin_start, shell_size);
    if (shell) sched_add(shell);

    __asm__ volatile ("sti");
    vga_puts("REPL ready. Type 'hello' or 'help'.\r\n");
    serial_puts("REPL ready.\r\n");
    /* Start first user task */
    sched_schedule();
}

void kernel_main(uint32_t magic, uint32_t info_phys)
{
    (void)magic;
    kernel_init_early((uintptr_t)info_phys);
    /* Main kernel loop - handle deferred scheduling */
    for (;;) {
        __asm__ volatile ("hlt");
        if (pit_need_sched()) {
            sched_schedule();
        }
    }
}
