#include "kernel/task/pcb.h"
#include "kernel/task/sched.h"
#include "kernel/drivers/pit.h"
#include <stddef.h>

uint8_t next_is_user;

static pcb_t *task_list;
static pcb_t *idle_task;

void sched_init(void)
{
    pcb_init();
    next_is_user = 0;
    task_list = NULL;
    idle_task = pcb_create_kernel(kernel_entry);
    if (idle_task) {
        idle_task->next = idle_task;
        task_list = idle_task;
        pcb_set_current(idle_task);
    }
}

void sched_add(pcb_t *p)
{
    if (!task_list) {
        task_list = p;
        p->next = p;
        return;
    }
    p->next = task_list->next;
    task_list->next = p;
}

void sched_schedule(void)
{
    if (!task_list) return;
    pcb_t *current = pcb_current();
    if (!current) current = idle_task;
    pcb_t *next = current->next;
    if (!next || next == current) return;
    task_list = next;
    next_is_user = next->is_user ? 1 : 0;
    /* Note: switch_to will save rsp to pcb_current, then set pcb_current = next */
    extern void switch_to(pcb_t *old, pcb_t *next);
    switch_to(current, next);
}

pcb_t *sched_current(void) { return pcb_current(); }
