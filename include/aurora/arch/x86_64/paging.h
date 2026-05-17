#ifndef AURORA_ARCH_X86_64_PAGING_H
#define AURORA_ARCH_X86_64_PAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    PAGE_SIZE_4K = 0x1000,
    PAGE_SIZE_2M = 0x200000,
};

enum {
    PAGE_PRESENT = 1ULL << 0,
    PAGE_WRITABLE = 1ULL << 1,
    PAGE_USER = 1ULL << 2,
    PAGE_WRITE_THROUGH = 1ULL << 3,
    PAGE_CACHE_DISABLE = 1ULL << 4,
    PAGE_ACCESSED = 1ULL << 5,
    PAGE_DIRTY = 1ULL << 6,
    PAGE_HUGE = 1ULL << 7,
    PAGE_GLOBAL = 1ULL << 8,
    PAGE_NX = 1ULL << 63,
};

typedef struct {
    uint64_t pml4_phys;
    uint64_t mapped_bytes;
} paging_stats_t;

void paging_init(void);
bool paging_map_2m(uint64_t virt, uint64_t phys, uint64_t flags);
uint64_t paging_root(void);
paging_stats_t paging_stats(void);

#endif
