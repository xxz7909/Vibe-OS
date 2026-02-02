#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#define SYS_READ  0
#define SYS_WRITE 1
#define SYS_EXIT  60
#define SYS_NET_SEND 10
#define SYS_NET_RECV 11

static inline long syscall1(long n, long a1)
{
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11");
    return ret;
}
static inline long syscall2(long n, long a1, long a2)
{
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11");
    return ret;
}
static inline long syscall3(long n, long a1, long a2, long a3)
{
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11");
    return ret;
}

static inline void user_write(int fd, const char *buf, unsigned long count)
{
    syscall3(SYS_WRITE, fd, (long)buf, count);
}
static inline long user_read(int fd, char *buf, unsigned long count)
{
    return syscall3(SYS_READ, fd, (long)buf, count);
}
static inline void user_exit(int code)
{
    (void)code;
    syscall1(SYS_EXIT, 0);
}
static inline int user_net_send(const void *buf, unsigned long len)
{
    return (int)syscall3(SYS_NET_SEND, (long)buf, (long)len, 0);
}
static inline int user_net_recv(void *buf, unsigned long max_len)
{
    return (int)syscall3(SYS_NET_RECV, (long)buf, (long)max_len, 0);
}

#endif
