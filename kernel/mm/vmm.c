#include <aurora/core/printk.h>
#include <aurora/mm/paging.h>
#include <aurora/mm/pmm.h>
#include <aurora/mm/vmm.h>

enum {
    VMM_BASE = 0xFFFFD00000000000ULL,
    VMM_LIMIT = 0xFFFFD10000000000ULL,
    VMM_PAGE_SIZE = 4096ULL
};

static uint64_t g_vmm_next = VMM_BASE;
static uint64_t g_vmm_reserved_bytes = 0ULL;

static uint64_t align_up_u64(uint64_t value, uint64_t alignment) {
    const uint64_t mask = alignment - 1ULL;
    return (value + mask) & ~mask;
}

void vmm_init(void) {
    g_vmm_next = VMM_BASE;
    g_vmm_reserved_bytes = 0ULL;
    printk("[aurora] vmm online base=0x%X limit=0x%X\n", VMM_BASE, VMM_LIMIT);
}

void *vmm_alloc(size_t size, uint64_t page_flags) {
    if (size == 0U) {
        return (void *)0;
    }

    const uint64_t aligned_size = align_up_u64((uint64_t)size, VMM_PAGE_SIZE);
    if (aligned_size == 0ULL) {
        return (void *)0;
    }

    const uint64_t start = g_vmm_next;
    const uint64_t end = start + aligned_size;
    if (end < start || end > VMM_LIMIT) {
        return (void *)0;
    }

    uint64_t mapped_pages = 0ULL;
    const uint64_t pages = aligned_size / VMM_PAGE_SIZE;
    for (; mapped_pages < pages; ++mapped_pages) {
        const uint64_t phys = pmm_alloc_page();
        if (phys == 0ULL) {
            break;
        }

        const uint64_t virt = start + (mapped_pages * VMM_PAGE_SIZE);
        if (!paging_map_page(virt, phys, page_flags | PAGE_PRESENT)) {
            pmm_free_page(phys);
            break;
        }
    }

    if (mapped_pages != pages) {
        for (uint64_t i = 0ULL; i < mapped_pages; ++i) {
            const uint64_t virt = start + (i * VMM_PAGE_SIZE);
            const uint64_t phys = paging_lookup_physical(virt);
            paging_unmap_page(virt);
            if (phys != 0ULL) {
                pmm_free_page(phys);
            }
        }
        return (void *)0;
    }

    g_vmm_next = end;
    g_vmm_reserved_bytes += aligned_size;
    return (void *)(uintptr_t)start;
}

void vmm_free(void *virtual_address, size_t size) {
    if (virtual_address == (void *)0 || size == 0U) {
        return;
    }

    const uint64_t start = (uint64_t)(uintptr_t)virtual_address;
    const uint64_t aligned_size = align_up_u64((uint64_t)size, VMM_PAGE_SIZE);
    const uint64_t pages = aligned_size / VMM_PAGE_SIZE;

    for (uint64_t i = 0ULL; i < pages; ++i) {
        const uint64_t virt = start + (i * VMM_PAGE_SIZE);
        const uint64_t phys = paging_lookup_physical(virt);
        paging_unmap_page(virt);
        if (phys != 0ULL) {
            pmm_free_page(phys);
        }
    }

    if (g_vmm_reserved_bytes >= aligned_size) {
        g_vmm_reserved_bytes -= aligned_size;
    } else {
        g_vmm_reserved_bytes = 0ULL;
    }
}

uint64_t vmm_bytes_reserved(void) {
    return g_vmm_reserved_bytes;
}
