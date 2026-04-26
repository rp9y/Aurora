#include <aurora/drivers/serial.h>
#include <aurora/lib/string.h>
#include <aurora/mm/paging.h>
#include <aurora/mm/pmm.h>

enum {
    PAGE_SIZE_4K = 4096ULL,
    PAGE_TABLE_ENTRIES = 512ULL,
    PAGE_ADDR_MASK = 0x000FFFFFFFFFF000ULL,
    PAGE_FLAG_MASK = PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER | PAGE_NO_EXECUTE,
    PAGE_HUGE = 1ULL << 7U,
    KERNEL_HIGHER_HALF_BASE = 0xFFFFFFFF80000000ULL
};

static uint64_t *g_pml4 = (uint64_t *)0;

static uint64_t align_down(uint64_t value, uint64_t alignment) {
    return value & ~(alignment - 1ULL);
}

static uint64_t align_up(uint64_t value, uint64_t alignment) {
    const uint64_t mask = alignment - 1ULL;
    return (value + mask) & ~mask;
}

static uint64_t read_cr3(void) {
    uint64_t value = 0U;
    __asm__ volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

static void invlpg(uint64_t virtual_addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

static uint64_t *phys_to_virt_table(uint64_t phys_addr) {
    return (uint64_t *)(uintptr_t)(phys_addr & PAGE_ADDR_MASK);
}

static uint64_t *paging_get_or_create_table(uint64_t *table, uint16_t index, uint64_t inherit_flags) {
    uint64_t entry = table[index];

    if ((entry & PAGE_PRESENT) != 0ULL) {
        if ((entry & PAGE_HUGE) != 0ULL) {
            return (uint64_t *)0;
        }
        return phys_to_virt_table(entry);
    }

    const uint64_t phys = pmm_alloc_page();
    if (phys == 0ULL) {
        return (uint64_t *)0;
    }

    uint64_t *new_table = phys_to_virt_table(phys);
    aurora_memset(new_table, 0, PAGE_SIZE_4K);

    uint64_t flags = PAGE_PRESENT | PAGE_WRITABLE;
    if ((inherit_flags & PAGE_USER) != 0ULL) {
        flags |= PAGE_USER;
    }

    table[index] = (phys & PAGE_ADDR_MASK) | flags;
    return new_table;
}

bool paging_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    if (g_pml4 == (uint64_t *)0) {
        return false;
    }

    const uint64_t vaddr = align_down(virtual_addr, PAGE_SIZE_4K);
    const uint64_t paddr = align_down(physical_addr, PAGE_SIZE_4K);

    const uint16_t pml4_index = (uint16_t)((vaddr >> 39U) & 0x1FFU);
    const uint16_t pdpt_index = (uint16_t)((vaddr >> 30U) & 0x1FFU);
    const uint16_t pd_index = (uint16_t)((vaddr >> 21U) & 0x1FFU);
    const uint16_t pt_index = (uint16_t)((vaddr >> 12U) & 0x1FFU);

    uint64_t *pdpt = paging_get_or_create_table(g_pml4, pml4_index, flags);
    if (pdpt == (uint64_t *)0) {
        return false;
    }

    uint64_t *pd = paging_get_or_create_table(pdpt, pdpt_index, flags);
    if (pd == (uint64_t *)0) {
        return false;
    }

    uint64_t *pt = paging_get_or_create_table(pd, pd_index, flags);
    if (pt == (uint64_t *)0) {
        return false;
    }

    pt[pt_index] = (paddr & PAGE_ADDR_MASK) | (flags & PAGE_FLAG_MASK) | PAGE_PRESENT;
    invlpg(vaddr);
    return true;
}

bool paging_map_range(uint64_t virtual_addr, uint64_t physical_addr, uint64_t length, uint64_t flags) {
    if (length == 0ULL) {
        return true;
    }

    const uint64_t start_virt = align_down(virtual_addr, PAGE_SIZE_4K);
    const uint64_t start_phys = align_down(physical_addr, PAGE_SIZE_4K);
    const uint64_t end_virt = align_up(virtual_addr + length, PAGE_SIZE_4K);
    const uint64_t page_count = (end_virt - start_virt) / PAGE_SIZE_4K;

    for (uint64_t i = 0ULL; i < page_count; ++i) {
        const uint64_t vaddr = start_virt + (i * PAGE_SIZE_4K);
        const uint64_t paddr = start_phys + (i * PAGE_SIZE_4K);
        if (!paging_map_page(vaddr, paddr, flags)) {
            for (uint64_t j = 0ULL; j < i; ++j) {
                paging_unmap_page(start_virt + (j * PAGE_SIZE_4K));
            }
            return false;
        }
    }

    return true;
}

void paging_unmap_page(uint64_t virtual_addr) {
    if (g_pml4 == (uint64_t *)0) {
        return;
    }

    const uint64_t vaddr = align_down(virtual_addr, PAGE_SIZE_4K);
    const uint16_t pml4_index = (uint16_t)((vaddr >> 39U) & 0x1FFU);
    const uint16_t pdpt_index = (uint16_t)((vaddr >> 30U) & 0x1FFU);
    const uint16_t pd_index = (uint16_t)((vaddr >> 21U) & 0x1FFU);
    const uint16_t pt_index = (uint16_t)((vaddr >> 12U) & 0x1FFU);

    if ((g_pml4[pml4_index] & PAGE_PRESENT) == 0ULL) {
        return;
    }

    uint64_t *pdpt = phys_to_virt_table(g_pml4[pml4_index]);
    if ((pdpt[pdpt_index] & PAGE_PRESENT) == 0ULL || (pdpt[pdpt_index] & PAGE_HUGE) != 0ULL) {
        return;
    }

    uint64_t *pd = phys_to_virt_table(pdpt[pdpt_index]);
    if ((pd[pd_index] & PAGE_PRESENT) == 0ULL || (pd[pd_index] & PAGE_HUGE) != 0ULL) {
        return;
    }

    uint64_t *pt = phys_to_virt_table(pd[pd_index]);
    pt[pt_index] = 0ULL;
    invlpg(vaddr);
}

void paging_unmap_range(uint64_t virtual_addr, uint64_t length) {
    if (length == 0ULL) {
        return;
    }

    const uint64_t start_virt = align_down(virtual_addr, PAGE_SIZE_4K);
    const uint64_t end_virt = align_up(virtual_addr + length, PAGE_SIZE_4K);

    for (uint64_t vaddr = start_virt; vaddr < end_virt; vaddr += PAGE_SIZE_4K) {
        paging_unmap_page(vaddr);
    }
}

uint64_t paging_lookup_physical(uint64_t virtual_addr) {
    if (g_pml4 == (uint64_t *)0) {
        return 0ULL;
    }

    const uint64_t vaddr = align_down(virtual_addr, PAGE_SIZE_4K);
    const uint16_t pml4_index = (uint16_t)((vaddr >> 39U) & 0x1FFU);
    const uint16_t pdpt_index = (uint16_t)((vaddr >> 30U) & 0x1FFU);
    const uint16_t pd_index = (uint16_t)((vaddr >> 21U) & 0x1FFU);
    const uint16_t pt_index = (uint16_t)((vaddr >> 12U) & 0x1FFU);

    if ((g_pml4[pml4_index] & PAGE_PRESENT) == 0ULL) {
        return 0ULL;
    }

    uint64_t *pdpt = phys_to_virt_table(g_pml4[pml4_index]);
    if ((pdpt[pdpt_index] & PAGE_PRESENT) == 0ULL || (pdpt[pdpt_index] & PAGE_HUGE) != 0ULL) {
        return 0ULL;
    }

    uint64_t *pd = phys_to_virt_table(pdpt[pdpt_index]);
    if ((pd[pd_index] & PAGE_PRESENT) == 0ULL || (pd[pd_index] & PAGE_HUGE) != 0ULL) {
        return 0ULL;
    }

    uint64_t *pt = phys_to_virt_table(pd[pd_index]);
    if ((pt[pt_index] & PAGE_PRESENT) == 0ULL) {
        return 0ULL;
    }

    return pt[pt_index] & PAGE_ADDR_MASK;
}

void paging_init(uintptr_t kernel_start, uintptr_t kernel_end) {
    const uint64_t cr3 = read_cr3();
    g_pml4 = phys_to_virt_table(cr3);

    const uint64_t aligned_start = align_down((uint64_t)kernel_start, PAGE_SIZE_4K);
    const uint64_t aligned_end = align_up((uint64_t)kernel_end, PAGE_SIZE_4K);

    for (uint64_t phys = aligned_start; phys < aligned_end; phys += PAGE_SIZE_4K) {
        const uint64_t virt = KERNEL_HIGHER_HALF_BASE + (phys - aligned_start);
        const bool mapped = paging_map_page(virt, phys, PAGE_WRITABLE);
        if (!mapped) {
            serial_write("[aurora] paging map failure\n");
            break;
        }
    }

    serial_write("[aurora] paging online (cr3=");
    serial_write_hex_u64(cr3 & PAGE_ADDR_MASK);
    serial_write(")\n");
}
