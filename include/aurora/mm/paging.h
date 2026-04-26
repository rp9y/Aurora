#ifndef AURORA_MM_PAGING_H
#define AURORA_MM_PAGING_H

#include <aurora/core/types.h>

#define PAGE_PRESENT ((uint64_t)1ULL << 0U)
#define PAGE_WRITABLE ((uint64_t)1ULL << 1U)
#define PAGE_USER ((uint64_t)1ULL << 2U)
#define PAGE_NO_EXECUTE ((uint64_t)1ULL << 63U)

void paging_init(uintptr_t kernel_start, uintptr_t kernel_end);
bool paging_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
bool paging_map_range(uint64_t virtual_addr, uint64_t physical_addr, uint64_t length, uint64_t flags);
void paging_unmap_page(uint64_t virtual_addr);
void paging_unmap_range(uint64_t virtual_addr, uint64_t length);
uint64_t paging_lookup_physical(uint64_t virtual_addr);

#endif
