/* IDE PIO: primary master at 0x1F0, 28-bit LBA */
#include "kernel/drivers/ide.h"
#include <stddef.h>

#define IDE_DATA    0x1F0
#define IDE_ERROR   0x1F1
#define IDE_SECT    0x1F2
#define IDE_LBA0    0x1F3
#define IDE_LBA1    0x1F4
#define IDE_LBA2    0x1F5
#define IDE_DEV     0x1F6
#define IDE_CMD     0x1F7
#define IDE_STATUS  0x1F7
#define IDE_CTL     0x3F6

#define CMD_READ    0x20
#define CMD_WRITE   0x30
#define STATUS_BSY  0x80
#define STATUS_DRQ  0x08
#define STATUS_ERR  0x01

static inline uint8_t inb(uint16_t port)
{
    uint8_t v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static inline void outb(uint16_t port, uint8_t v)
{
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}
static inline void outw(uint16_t port, uint16_t v)
{
    __asm__ volatile ("outw %0, %1" : : "a"(v), "Nd"(port));
}
static inline void insw(uint16_t port, void *buf, size_t count)
{
    __asm__ volatile ("rep insw" : "+D"(buf), "+c"(count) : "d"(port) : "memory");
}
static inline void outsw(uint16_t port, const void *buf, size_t count)
{
    __asm__ volatile ("rep outsw" : "+S"(buf), "+c"(count) : "d"(port) : "memory");
}

void ide_init(void)
{
    outb(IDE_CTL, 0);
    outb(IDE_DEV, 0xE0 | 0);
    /* Wait for BSY to clear with timeout */
    for (int i = 0; i < 100000; i++) {
        if (!(inb(IDE_STATUS) & STATUS_BSY)) break;
    }
}

static bool wait_drq(void)
{
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb(IDE_STATUS);
        if (s & STATUS_ERR) return false;
        if (s & STATUS_DRQ) return true;
        if (!(s & STATUS_BSY)) return false;
    }
    return false;
}

bool block_read(int dev, uint32_t lba, void *buf)
{
    (void)dev;
    outb(IDE_DEV, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IDE_SECT, 1);
    outb(IDE_LBA0, lba & 0xFF);
    outb(IDE_LBA1, (lba >> 8) & 0xFF);
    outb(IDE_LBA2, (lba >> 16) & 0xFF);
    outb(IDE_CMD, CMD_READ);
    if (!wait_drq()) return false;
    insw(IDE_DATA, buf, 256);
    return true;
}

bool block_write(int dev, uint32_t lba, const void *buf)
{
    (void)dev;
    outb(IDE_DEV, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IDE_SECT, 1);
    outb(IDE_LBA0, lba & 0xFF);
    outb(IDE_LBA1, (lba >> 8) & 0xFF);
    outb(IDE_LBA2, (lba >> 16) & 0xFF);
    outb(IDE_CMD, CMD_WRITE);
    if (!wait_drq()) return false;
    outsw(IDE_DATA, buf, 256);
    for (int i = 0; i < 1000000; i++)
        if (!(inb(IDE_STATUS) & STATUS_BSY)) return true;
    return false;
}
