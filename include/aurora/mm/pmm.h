#ifndef AURORA_MM_PMM_H
#define AURORA_MM_PMM_H

#include <aurora/core/types.h>

void pmm_init(uintptr_t multiboot_info_addr, uintptr_t kernel_start, uintptr_t kernel_end);
void pmm_init_fallback(uint64_t free_region_start, uint64_t free_region_end, uintptr_t kernel_start, uintptr_t kernel_end);
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t phys_addr);
uint64_t pmm_total_pages(void);
uint64_t pmm_free_pages(void);

#endif
