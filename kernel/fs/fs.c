/* Minimal FS: superblock + inode table + data blocks on block device */
#include "kernel/fs/fs.h"
#include "kernel/drivers/ide.h"
#include "lib/string.h"

static int fs_dev;
static uint32_t fs_start_lba;
static fs_super_t super;
static fs_inode_t inodes[FS_MAX_FILES];
static bool mounted;
static uint8_t block_buf[FS_BLOCK_SIZE];

#define INODES_PER_BLOCK (FS_BLOCK_SIZE / sizeof(fs_inode_t))

void fs_init(void)
{
    mounted = false;
}

static bool read_block(uint32_t lba, void *buf)
{
    return block_read(fs_dev, fs_start_lba + lba, buf);
}

bool fs_mount(int dev, uint32_t start_lba)
{
    fs_dev = dev;
    fs_start_lba = start_lba;
    if (!read_block(0, &super) || super.magic != FS_MAGIC)
        return false;
#define INODES_PER_BLOCK (FS_BLOCK_SIZE / sizeof(fs_inode_t))
    uint32_t inode_blocks = (super.inode_count + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
    for (uint32_t i = 0; i < inode_blocks; i++) {
        if (!read_block(1 + i, block_buf)) return false;
        size_t start = i * INODES_PER_BLOCK;
        size_t count = INODES_PER_BLOCK;
        if (start + count > super.inode_count) count = super.inode_count - start;
        memcpy(&inodes[start], block_buf, count * sizeof(fs_inode_t));
    }
#undef INODES_PER_BLOCK
    mounted = true;
    return true;
}

static int find_inode_by_name(const char *path)
{
    if (!mounted || !path) return -1;
    for (uint32_t i = 0; i < super.inode_count && i < FS_MAX_FILES; i++) {
        if (inodes[i].type != FS_INODE_FILE && inodes[i].type != FS_INODE_DIR) continue;
        if (strcmp(path, inodes[i].name) == 0) return (int)i;
    }
    return -1;
}

int fs_open(const char *path)
{
    return find_inode_by_name(path);
}

void fs_close(int fd) { (void)fd; }

int fs_read(int fd, void *buf, size_t count)
{
    if (!mounted || fd < 0 || (uint32_t)fd >= super.inode_count) return -1;
    fs_inode_t *in = &inodes[fd];
    if (in->type != FS_INODE_FILE) return -1;
    size_t off = 0;
    size_t total = count;
    if (total > in->size) total = in->size;
    for (uint32_t i = 0; i < FS_MAX_BLOCKS && off < total; i++) {
        if (in->blocks[i] == 0) break;
        uint32_t inode_blocks = (super.inode_count * sizeof(fs_inode_t) + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        if (!read_block(1 + inode_blocks + in->blocks[i], block_buf))
            break;
        size_t chunk = FS_BLOCK_SIZE;
        if (off + chunk > total) chunk = total - off;
        memcpy((char *)buf + off, block_buf, chunk);
        off += chunk;
    }
    return (int)off;
}

int fs_list(char *buf, size_t max)
{
    if (!mounted) return -1;
    size_t n = 0;
    for (uint32_t i = 0; i < super.inode_count && i < FS_MAX_FILES && n < max - 1; i++) {
        if (inodes[i].type == FS_INODE_FREE) continue;
        size_t len = strlen(inodes[i].name);
        if (n + len + 2 > max) break;
        memcpy(buf + n, inodes[i].name, len);
        n += len;
        buf[n++] = '\n';
    }
    buf[n] = '\0';
    return (int)n;
}
