#ifndef VMM_H
#define VMM_H
#include <stdint.h>
#include <stdbool.h>

void vmm_init(void);
bool vmm_map_page(void *pml4, uintptr_t vaddr, uintptr_t paddr, bool user, bool writable);
void vmm_unmap_page(void *pml4, uintptr_t vaddr);
void *vmm_new_user_pml4(void);
void *vmm_get_kernel_pml4(void);
void *vmm_map_io(uintptr_t phys_addr);

#endif
