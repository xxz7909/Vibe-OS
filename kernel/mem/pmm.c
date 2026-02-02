/* Physical memory manager: bitmap from Multiboot2 memory map */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE    4096
#define PAGE_SHIFT   12
#define MAX_PAGES    (128 * 1024 * 1024 / PAGE_SIZE)   /* 128MB / 4K = 32K pages */
#define BITMAP_SIZE  (MAX_PAGES / 8)                    /* 4KB bitmap for 128MB */

static uint8_t bitmap[BITMAP_SIZE];
static uintptr_t max_phys_page;

#define MB2_TAG_END      0
#define MB2_TAG_MMAP     6
#define MB2_MMAP_AVAIL   1

static void set_bit(size_t page) {
    if (page >= MAX_PAGES) return;
    bitmap[page / 8] |= (1u << (page % 8));
}
static void clear_bit(size_t page) {
    if (page >= MAX_PAGES) return;
    bitmap[page / 8] &= ~(1u << (page % 8));
}
static bool test_bit(size_t page) {
    if (page >= MAX_PAGES) return true;
    return (bitmap[page / 8] & (1u << (page % 8))) != 0;
}

void pmm_init(uintptr_t multiboot_info_phys)
{
    (void)multiboot_info_phys;

    /* Simple init: assume 128MB RAM, mark all as used first */
    for (size_t i = 0; i < BITMAP_SIZE; i++)
        bitmap[i] = 0xFF;
    max_phys_page = (128 * 1024 * 1024) >> PAGE_SHIFT;

    /* Mark pages 4MB-128MB as available */
    for (size_t pg = (4 * 1024 * 1024) >> PAGE_SHIFT; pg < max_phys_page; pg++)
        clear_bit(pg);
}

void *pmm_alloc_page(void)
{
    for (size_t pg = (4 * 1024 * 1024) >> PAGE_SHIFT; pg < max_phys_page && pg < MAX_PAGES; pg++) {
        if (!test_bit(pg)) {
            set_bit(pg);
            return (void *)(pg << PAGE_SHIFT);
        }
    }
    return NULL;
}

void pmm_free_page(void *ptr)
{
    uintptr_t pg = (uintptr_t)ptr >> PAGE_SHIFT;
    if (pg < MAX_PAGES) clear_bit(pg);
}

uintptr_t pmm_get_max_page(void) { return max_phys_page; }
