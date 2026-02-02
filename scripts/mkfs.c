/* Host tool: create fs.img with superblock + inodes + data (shell.bin, hello.bin) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FS_BLOCK_SIZE 512
#define FS_MAGIC      0x0F510F51
#define FS_MAX_NAME   32
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

#define FS_INODE_FILE 1

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "Usage: mkfs <fs.img> <shell.bin> <hello.bin>\n");
        return 1;
    }
    FILE *shell = fopen(argv[2], "rb");
    FILE *hello = fopen(argv[3], "rb");
    if (!shell || !hello) {
        perror("fopen");
        return 1;
    }
    fseek(shell, 0, SEEK_END);
    long shell_len = ftell(shell);
    fseek(shell, 0, SEEK_SET);
    fseek(hello, 0, SEEK_END);
    long hello_len = ftell(hello);
    fseek(hello, 0, SEEK_SET);

    uint32_t inode_blocks = (2 * sizeof(fs_inode_t) + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    uint32_t shell_blocks = (shell_len + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    uint32_t hello_blocks = (hello_len + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    uint32_t data_start = 1 + inode_blocks;
    uint32_t total_blocks = data_start + shell_blocks + hello_blocks;

    FILE *out = fopen(argv[1], "wb");
    if (!out) {
        perror("fopen out");
        return 1;
    }

    uint8_t block[FS_BLOCK_SIZE];
    memset(block, 0, sizeof(block));
    fs_super_t *super = (fs_super_t *)block;
    super->magic = FS_MAGIC;
    super->block_count = total_blocks;
    super->inode_count = 2;
    super->root_inode = 0;
    fwrite(block, 1, FS_BLOCK_SIZE, out);

    memset(block, 0, sizeof(block));
    fs_inode_t *in0 = (fs_inode_t *)block;
    in0->type = FS_INODE_FILE;
    strcpy(in0->name, "shell.bin");
    in0->name_len = 9;
    in0->size = shell_len;
    in0->blocks[0] = 0;
    for (uint32_t i = 1; i < shell_blocks && i < FS_MAX_BLOCKS; i++) in0->blocks[i] = i;
    fs_inode_t *in1 = (fs_inode_t *)(block + sizeof(fs_inode_t));
    in1->type = FS_INODE_FILE;
    strcpy(in1->name, "hello.bin");
    in1->name_len = 9;
    in1->size = hello_len;
    for (uint32_t i = 0; i < hello_blocks && i < FS_MAX_BLOCKS; i++) in1->blocks[i] = shell_blocks + i;
    fwrite(block, 1, FS_BLOCK_SIZE, out);

    for (uint32_t i = 0; i < shell_blocks; i++) {
        memset(block, 0, sizeof(block));
        size_t n = fread(block, 1, FS_BLOCK_SIZE, shell);
        fwrite(block, 1, FS_BLOCK_SIZE, out);
        (void)n;
    }
    for (uint32_t i = 0; i < hello_blocks; i++) {
        memset(block, 0, sizeof(block));
        size_t n = fread(block, 1, FS_BLOCK_SIZE, hello);
        fwrite(block, 1, FS_BLOCK_SIZE, out);
        (void)n;
    }

    fclose(out);
    fclose(shell);
    fclose(hello);
    printf("Created %s: %u blocks\n", argv[1], total_blocks);
    return 0;
}
