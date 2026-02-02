#include "kernel/task/pcb.h"
#include "kernel/mem/pmm.h"
#include "kernel/mem/vmm.h"
#include <stddef.h>
#include "lib/string.h"

#define MAX_TASKS 16
static pcb_t tasks[MAX_TASKS];
static int num_tasks;
pcb_t *current_pcb;  /* global for asm: syscall_entry, switch_to */

void pcb_init(void)
{
    num_tasks = 0;
    current_pcb = NULL;
}

pcb_t *pcb_current(void) { return current_pcb; }
void pcb_set_current(pcb_t *p) { current_pcb = p; }

void kernel_entry(void) { for (;;) __asm__ volatile ("hlt"); }

pcb_t *pcb_create_kernel(void (*entry)(void))
{
    if (num_tasks >= MAX_TASKS) return NULL;
    pcb_t *p = &tasks[num_tasks++];
    p->cr3 = vmm_get_kernel_pml4();
    p->is_user = false;
    p->state = TASK_READY;
    p->id = num_tasks - 1;
    p->next = NULL;
    void *kstack = pmm_alloc_page();
    if (!kstack) return NULL;
    uint64_t *sp = (uint64_t *)((char *)kstack + KERNEL_STACK_SIZE);
    /* Stack frame for kernel task: return address + callee-saved registers */
    *--sp = entry ? (uint64_t)entry : 0;  /* return address */
    *--sp = 0;  /* r15 */
    *--sp = 0;  /* r14 */
    *--sp = 0;  /* r13 */
    *--sp = 0;  /* r12 */
    *--sp = 0;  /* rbp */
    *--sp = 0;  /* rbx */
    p->kernel_rsp = (uint64_t)sp;
    return p;
}

pcb_t *pcb_create_user(void *code, size_t code_size)
{
    if (num_tasks >= MAX_TASKS) return NULL;
    pcb_t *p = &tasks[num_tasks++];
    p->cr3 = vmm_new_user_pml4();
    if (!p->cr3) return NULL;
    p->is_user = true;
    p->state = TASK_READY;
    p->id = num_tasks - 1;
    p->next = NULL;

    uintptr_t user_code = USER_ENTRY;
    uintptr_t user_stack = USER_STACK_TOP - 4096;
    for (size_t i = 0; i < (code_size + 4095) / 4096; i++)
        vmm_map_page(p->cr3, user_code + i * 4096, (uintptr_t)pmm_alloc_page(), true, true);
    uintptr_t old_cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile ("mov %0, %%cr3" : : "r"((uintptr_t)p->cr3));
    memcpy((void *)user_code, code, code_size);
    __asm__ volatile ("mov %0, %%cr3" : : "r"(old_cr3));

    vmm_map_page(p->cr3, user_stack, (uintptr_t)pmm_alloc_page(), true, true);
    p->user_rsp = user_stack + 4096;
    p->user_rip = user_code;

    void *kstack = pmm_alloc_page();
    if (!kstack) return NULL;
    uint64_t *sp = (uint64_t *)((char *)kstack + KERNEL_STACK_SIZE);
    uint64_t user_cs = 0x23, user_ss = 0x1b, user_rflags = 0x202;  /* GDT: code=0x20|3, data=0x18|3 */
    /* Build iret frame: ss, rsp, rflags, cs, rip (pushed in reverse order) */
    *--sp = user_ss;           /* SS */
    *--sp = p->user_rsp;       /* RSP */
    *--sp = user_rflags;       /* RFLAGS */
    *--sp = user_cs;           /* CS */
    *--sp = p->user_rip;       /* RIP */
    /* Push registers in REVERSE order of switch.asm pop sequence */
    /* switch.asm pops: r15,r14,r13,r12,r11,r10,r9,r8,rbp,rdi,rsi,rdx,rcx,rbx,rax */
    /* So we push: rax,rbx,rcx,rdx,rsi,rdi,rbp,r8,r9,r10,r11,r12,r13,r14,r15 */
    *--sp = 0;  /* rax - popped last */
    *--sp = 0;  /* rbx */
    *--sp = 0;  /* rcx */
    *--sp = 0;  /* rdx */
    *--sp = 0;  /* rsi */
    *--sp = 0;  /* rdi */
    *--sp = 0;  /* rbp */
    *--sp = 0;  /* r8 */
    *--sp = 0;  /* r9 */
    *--sp = 0;  /* r10 */
    *--sp = 0;  /* r11 */
    *--sp = 0;  /* r12 */
    *--sp = 0;  /* r13 */
    *--sp = 0;  /* r14 */
    *--sp = 0;  /* r15 - popped first */
    p->kernel_rsp = (uint64_t)sp;
    return p;
}
