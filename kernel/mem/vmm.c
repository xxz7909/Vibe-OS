/* Virtual memory: 4-level paging, map_page / unmap_page */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kernel/mem/pmm.h"

#define PAGE_SIZE    4096
#define PAGE_SHIFT   12
#define PTE_P        1
#define PTE_W        2
#define PTE_U        4
#define PTE_PS       0x80   /* huge page (PD level) */

typedef uint64_t pte_t;
#define PT_ENTRIES   512

static pte_t *kernel_pml4;

static pte_t *get_pt(void)
{
    void *p = pmm_alloc_page();
    if (!p) return NULL;
    for (int i = 0; i < 512; i++)
        ((pte_t *)p)[i] = 0;
    return (pte_t *)p;
}

void vmm_init(void)
{
    kernel_pml4 = (pte_t *)pmm_alloc_page();
    if (!kernel_pml4) return;
    for (int i = 0; i < 512; i++) kernel_pml4[i] = 0;

    /* Identity map first 16MB (kernel + allocatable memory) using 2MB huge pages */
    pte_t *pdp = (pte_t *)pmm_alloc_page();
    if (!pdp) return;
    for (int i = 0; i < 512; i++) pdp[i] = 0;
    kernel_pml4[0] = (uintptr_t)pdp | PTE_P | PTE_W;

    pte_t *pd = (pte_t *)pmm_alloc_page();
    if (!pd) return;
    for (int i = 0; i < 512; i++) pd[i] = 0;
    pdp[0] = (uintptr_t)pd | PTE_P | PTE_W;

    /* Map 8 * 2MB = 16MB using huge pages (PS bit) */
    for (int i = 0; i < 8; i++)
        pd[i] = (i * 0x200000) | PTE_P | PTE_W | PTE_PS;

    __asm__ volatile ("mov %0, %%cr3" : : "r"((uintptr_t)kernel_pml4));
}

/* Map one page: vaddr -> paddr, user accessible if user=true */
bool vmm_map_page(pte_t *pml4, uintptr_t vaddr, uintptr_t paddr, bool user, bool writable)
{
    int pml4_i = (vaddr >> 39) & 0x1FF;
    int pdp_i  = (vaddr >> 30) & 0x1FF;
    int pd_i   = (vaddr >> 21) & 0x1FF;
    int pt_i   = (vaddr >> 12) & 0x1FF;

    /* All levels need PTE_U if user mode access is required */
    uint64_t dir_flags = PTE_P | PTE_W | (user ? PTE_U : 0);

    if (!pml4[pml4_i]) {
        pte_t *p = get_pt();
        if (!p) return false;
        pml4[pml4_i] = (uintptr_t)p | dir_flags;
    } else if (user && !(pml4[pml4_i] & PTE_U)) {
        pml4[pml4_i] |= PTE_U;
    }
    pte_t *pdp = (pte_t *)(pml4[pml4_i] & ~0xFFF);
    if (!pdp[pdp_i]) {
        pte_t *p = get_pt();
        if (!p) return false;
        pdp[pdp_i] = (uintptr_t)p | dir_flags;
    } else if (user && !(pdp[pdp_i] & PTE_U)) {
        pdp[pdp_i] |= PTE_U;
    }
    pte_t *pd = (pte_t *)(pdp[pdp_i] & ~0xFFF);
    if (!pd[pd_i]) {
        pte_t *p = get_pt();
        if (!p) return false;
        pd[pd_i] = (uintptr_t)p | dir_flags;
    } else if (user && !(pd[pd_i] & PTE_U)) {
        pd[pd_i] |= PTE_U;
    }
    pte_t *pt = (pte_t *)(pd[pd_i] & ~0xFFF);
    uint64_t flags = PTE_P | (writable ? PTE_W : 0) | (user ? PTE_U : 0);
    pt[pt_i] = (paddr & ~0xFFF) | flags;
    return true;
}

void vmm_unmap_page(pte_t *pml4, uintptr_t vaddr)
{
    int pml4_i = (vaddr >> 39) & 0x1FF;
    int pdp_i  = (vaddr >> 30) & 0x1FF;
    int pd_i   = (vaddr >> 21) & 0x1FF;
    int pt_i   = (vaddr >> 12) & 0x1FF;
    if (!pml4[pml4_i]) return;
    pte_t *pdp = (pte_t *)(pml4[pml4_i] & ~0xFFF);
    if (!pdp[pdp_i]) return;
    pte_t *pd = (pte_t *)(pdp[pdp_i] & ~0xFFF);
    if (!pd[pd_i]) return;
    pte_t *pt = (pte_t *)(pd[pd_i] & ~0xFFF);
    pt[pt_i] = 0;
}

pte_t *vmm_new_user_pml4(void)
{
    pte_t *pml4 = (pte_t *)pmm_alloc_page();
    if (!pml4) return NULL;
    for (int i = 0; i < 512; i++) pml4[i] = 0;
    /* Copy kernel mappings (low half identity) from kernel_pml4 so kernel is mapped in user space too */
    for (int i = 0; i < 256; i++)
        if (kernel_pml4[i])
            pml4[i] = kernel_pml4[i];
    return pml4;
}

pte_t *vmm_get_kernel_pml4(void) { return kernel_pml4; }

/* For simplicity, use identity mapping for MMIO */
/* Only support addresses below 16MB (already identity-mapped) */
void *vmm_map_io(uintptr_t phys_addr)
{
    /* Only support addresses in the already identity-mapped region */
    if (phys_addr < 0x1000000) {
        return (void *)phys_addr;
    }
    /* For higher addresses (like PCI MMIO), return NULL - not supported yet */
    return NULL;
}
