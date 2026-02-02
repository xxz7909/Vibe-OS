#ifndef PCB_H
#define PCB_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define KERNEL_STACK_SIZE 4096
#define USER_ENTRY       0x10000000   /* 256MB - above kernel identity map */
#define USER_STACK_TOP   0x20000000   /* 512MB */

typedef enum { TASK_RUNNING, TASK_READY, TASK_BLOCKED } task_state_t;

typedef struct pcb {
    uint64_t kernel_rsp;      /* kernel stack top for context switch / syscall */
    void    *cr3;             /* page table */
    uint64_t user_rsp;        /* user stack pointer (for iret) */
    uint64_t user_rip;        /* user entry (for iret) */
    bool     is_user;         /* true => iret to user on switch */
    task_state_t state;
    int      id;
    struct pcb *next;
} pcb_t;

void pcb_init(void);
pcb_t *pcb_current(void);
void pcb_set_current(pcb_t *p);
pcb_t *pcb_create_user(void *code, size_t code_size);
pcb_t *pcb_create_kernel(void (*entry)(void));
void kernel_entry(void);

#endif
