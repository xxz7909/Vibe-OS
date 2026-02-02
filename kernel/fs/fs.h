#ifndef FS_H
#define FS_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define FS_BLOCK_SIZE 512
#define FS_MAGIC      0x0F510F51
#define FS_MAX_NAME   32
#define FS_MAX_FILES  64
#define FS_MAX_BLOCKS 8

typedef struct {
    uint32_t magic;
    uint32_t block_count;
    uint32_t inode_count;
    uint32_t root_inode;
} fs_super_t;

typedef struct {
    uint8_t  type;
    uint8_t  name_len;
    char     name[FS_MAX_NAME];
    uint32_t size;
    uint32_t blocks[FS_MAX_BLOCKS];
} fs_inode_t;

#define FS_INODE_FREE  0
#define FS_INODE_FILE  1
#define FS_INODE_DIR   2

void fs_init(void);
bool fs_mount(int dev, uint32_t start_lba);
int fs_open(const char *path);
void fs_close(int fd);
int fs_read(int fd, void *buf, size_t count);
int fs_list(char *buf, size_t max);

#endif
