#ifndef IDE_H
#define IDE_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void ide_init(void);
bool block_read(int dev, uint32_t lba, void *buf);
bool block_write(int dev, uint32_t lba, const void *buf);

#endif
