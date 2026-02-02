#ifndef PMM_H
#define PMM_H
#include <stdint.h>
#include <stddef.h>

void pmm_init(uintptr_t multiboot_info_phys);
void *pmm_alloc_page(void);
void pmm_free_page(void *ptr);
uintptr_t pmm_get_max_page(void);

#endif
