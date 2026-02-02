#include "syscall.h"

void _start(void)
{
    const char msg[] = "Hello from user program!\r\n";
    user_write(1, msg, sizeof(msg) - 1);
    user_exit(0);
}
