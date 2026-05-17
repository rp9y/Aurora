#include "aurora/arch/x86_64/paging.h"

#include "aurora/arch/x86_64/cpu.h"
#include "core/libk/string.h"

static uint64_t g_pml4[512] __attribute__((aligned(PAGE_SIZE_4K)));
static uint64_t g_pdpt[512] __attribute__((aligned(PAGE_SIZE_4K)));
static uint64_t g_pd0[512] __attribute__((aligned(PAGE_SIZE_4K)));

static paging_stats_t g_stats;

static inline uint64_t align_down_2m(uint64_t value) {
    return value & ~(uint64_t)(PAGE_SIZE_2M - 1ULL);
}

void paging_init(void) {
    kmemset(g_pml4, 0, sizeof(g_pml4));
    kmemset(g_pdpt, 0, sizeof(g_pdpt));
    kmemset(g_pd0, 0, sizeof(g_pd0));

    g_pml4[0] = ((uint64_t)(uintptr_t)g_pdpt) | PAGE_PRESENT | PAGE_WRITABLE;
    g_pdpt[0] = ((uint64_t)(uintptr_t)g_pd0) | PAGE_PRESENT | PAGE_WRITABLE;

    for (size_t i = 0; i < 512; i++) {
        const uint64_t phys = (uint64_t)i * PAGE_SIZE_2M;
        g_pd0[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE;
    }

    g_stats.pml4_phys = (uint64_t)(uintptr_t)g_pml4;
    g_stats.mapped_bytes = (uint64_t)512 * PAGE_SIZE_2M;

    cpu_write_cr3((uint64_t)(uintptr_t)g_pml4);
}

bool paging_map_2m(uint64_t virt, uint64_t phys, uint64_t flags) {
    const uint16_t pml4_index = (uint16_t)((virt >> 39) & 0x1FFu);
    const uint16_t pdpt_index = (uint16_t)((virt >> 30) & 0x1FFu);
    const uint16_t pd_index = (uint16_t)((virt >> 21) & 0x1FFu);
    if (pml4_index != 0 || pdpt_index != 0) {
        return false;
    }

    const uint64_t aligned_virt = align_down_2m(virt);
    const uint64_t aligned_phys = align_down_2m(phys);
    const uint64_t entry_flags = (flags | PAGE_PRESENT | PAGE_HUGE) & ~0x1FFFFFULL;

    (void)aligned_virt;
    g_pd0[pd_index] = aligned_phys | entry_flags;
    if (g_stats.mapped_bytes < (uint64_t)(pd_index + 1u) * PAGE_SIZE_2M) {
        g_stats.mapped_bytes = (uint64_t)(pd_index + 1u) * PAGE_SIZE_2M;
    }
    return true;
}

uint64_t paging_root(void) {
    return (uint64_t)(uintptr_t)g_pml4;
}

paging_stats_t paging_stats(void) {
    return g_stats;
}
