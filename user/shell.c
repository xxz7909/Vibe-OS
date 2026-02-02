#include "syscall.h"

static const char prompt[] = "vibe> ";
static const char bad[] = "unknown command (try hello / help)\r\n";
static const char hello_msg[] = "Hello vibe-OS!\r\n";
static const char help_msg[] = "Commands: hello, help\r\n";

/* Forward declaration so _start can be defined first */
static int match(const char *buf, const char *cmd);

/* _start must be first function for correct entry point */
void __attribute__((section(".text._start"))) _start(void)
{
    char buf[64];
    for (;;) {
        user_write(1, prompt, sizeof(prompt) - 1);
        long n = user_read(0, buf, sizeof(buf) - 1);
        if (n <= 0) continue;
        buf[n] = '\0';
        if (match(buf, "hello"))
            user_write(1, hello_msg, sizeof(hello_msg) - 1);
        else if (match(buf, "help"))
            user_write(1, help_msg, sizeof(help_msg) - 1);
        else
            user_write(1, bad, sizeof(bad) - 1);
    }
}

static int match(const char *buf, const char *cmd)
{
    for (; *cmd; buf++, cmd++)
        if (*buf != *cmd) return 0;
    return *buf == '\0' || *buf == '\n' || *buf == '\r';
}
