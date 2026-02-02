#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stddef.h>

#define SYS_READ    0
#define SYS_WRITE   1
#define SYS_NET_SEND 10
#define SYS_NET_RECV 11
#define SYS_EXIT    60

void syscall_init(void);
void syscall_handler(void *frame_ptr);

#endif
