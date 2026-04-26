#include <aurora/boot/multiboot2.h>
#include <aurora/drivers/serial.h>
#include <aurora/lib/string.h>
#include <aurora/mm/pmm.h>

enum {
    PMM_PAGE_SIZE = 4096ULL,
    PMM_MAX_PHYS_ADDR = 0x100000000ULL,
    PMM_MAX_PAGES = PMM_MAX_PHYS_ADDR / PMM_PAGE_SIZE,
    PMM_BITMAP_WORDS = PMM_MAX_PAGES / 64ULL
};

static uint64_t g_bitmap[PMM_BITMAP_WORDS];
static uint64_t g_total_pages = PMM_MAX_PAGES;
static uint64_t g_free_pages = 0U;

static uint64_t align_up(uint64_t value, uint64_t alignment) {
    const uint64_t mask = alignment - 1ULL;
    return (value + mask) & ~mask;
}

static uint64_t align_down(uint64_t value, uint64_t alignment) {
    return value & ~(alignment - 1ULL);
}

static bool pmm_page_valid(uint64_t page) {
    return page < g_total_pages;
}

static bool pmm_bit_test(uint64_t page) {
    const uint64_t word = page / 64ULL;
    const uint64_t bit = page % 64ULL;
    return (g_bitmap[word] & (1ULL << bit)) != 0ULL;
}

static void pmm_bit_set(uint64_t page) {
    const uint64_t word = page / 64ULL;
    const uint64_t bit = page % 64ULL;
    const uint64_t mask = 1ULL << bit;
    if ((g_bitmap[word] & mask) == 0ULL) {
        g_bitmap[word] |= mask;
        if (g_free_pages > 0ULL) {
            --g_free_pages;
        }
    }
}

static void pmm_bit_clear(uint64_t page) {
    const uint64_t word = page / 64ULL;
    const uint64_t bit = page % 64ULL;
    const uint64_t mask = 1ULL << bit;
    if ((g_bitmap[word] & mask) != 0ULL) {
        g_bitmap[word] &= ~mask;
        ++g_free_pages;
    }
}

static void pmm_mark_range_free(uint64_t addr, uint64_t size) {
    const uint64_t start = align_up(addr, PMM_PAGE_SIZE);
    const uint64_t end = align_down(addr + size, PMM_PAGE_SIZE);

    for (uint64_t current = start; current < end; current += PMM_PAGE_SIZE) {
        const uint64_t page = current / PMM_PAGE_SIZE;
        if (pmm_page_valid(page)) {
            pmm_bit_clear(page);
        }
    }
}

static void pmm_mark_range_used(uint64_t addr, uint64_t size) {
    const uint64_t start = align_down(addr, PMM_PAGE_SIZE);
    const uint64_t end = align_up(addr + size, PMM_PAGE_SIZE);

    for (uint64_t current = start; current < end; current += PMM_PAGE_SIZE) {
        const uint64_t page = current / PMM_PAGE_SIZE;
        if (pmm_page_valid(page)) {
            pmm_bit_set(page);
        }
    }
}

void pmm_init(uintptr_t multiboot_info_addr, uintptr_t kernel_start, uintptr_t kernel_end) {
    aurora_memset(g_bitmap, 0xFF, sizeof(g_bitmap));
    g_total_pages = PMM_MAX_PAGES;
    g_free_pages = 0U;

    const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info_addr;
    const uintptr_t tags_start = multiboot_info_addr + 8U;
    const uintptr_t tags_end = multiboot_info_addr + (uintptr_t)total_size;

    uintptr_t cursor = tags_start;
    while (cursor < tags_end) {
        const multiboot_tag_t *tag = (const multiboot_tag_t *)cursor;
        if (tag->type == MULTIBOOT2_TAG_TYPE_END) {
            break;
        }

        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            const multiboot_tag_mmap_t *mmap_tag = (const multiboot_tag_mmap_t *)tag;
            const uint32_t entry_size = mmap_tag->entry_size;
            uintptr_t entry_cursor = (uintptr_t)&mmap_tag->entries[0];
            const uintptr_t entry_end = cursor + mmap_tag->size;

            while (entry_cursor + entry_size <= entry_end) {
                const multiboot_mmap_entry_t *entry = (const multiboot_mmap_entry_t *)entry_cursor;
                if (entry->type == 1U) {
                    pmm_mark_range_free(entry->addr, entry->len);
                }
                entry_cursor += entry_size;
            }
        }

        cursor += (uintptr_t)((tag->size + 7U) & ~7U);
    }

    pmm_mark_range_used(0U, 0x100000ULL);
    pmm_mark_range_used((uint64_t)kernel_start, (uint64_t)(kernel_end - kernel_start));
    pmm_mark_range_used((uint64_t)multiboot_info_addr, (uint64_t)total_size);

    serial_write("[aurora] pmm pages total=");
    serial_write_dec_u64(g_total_pages);
    serial_write(" free=");
    serial_write_dec_u64(g_free_pages);
    serial_write_char('\n');
}

void pmm_init_fallback(uint64_t free_region_start, uint64_t free_region_end, uintptr_t kernel_start, uintptr_t kernel_end) {
    aurora_memset(g_bitmap, 0xFF, sizeof(g_bitmap));
    g_total_pages = PMM_MAX_PAGES;
    g_free_pages = 0U;

    if (free_region_end > free_region_start) {
        pmm_mark_range_free(free_region_start, free_region_end - free_region_start);
    }

    pmm_mark_range_used(0U, 0x100000ULL);
    pmm_mark_range_used((uint64_t)kernel_start, (uint64_t)(kernel_end - kernel_start));

    serial_write("[aurora] pmm fallback pages total=");
    serial_write_dec_u64(g_total_pages);
    serial_write(" free=");
    serial_write_dec_u64(g_free_pages);
    serial_write_char('\n');
}

uint64_t pmm_alloc_page(void) {
    for (uint64_t page = 0ULL; page < g_total_pages; ++page) {
        if (!pmm_bit_test(page)) {
            pmm_bit_set(page);
            return page * PMM_PAGE_SIZE;
        }
    }
    return 0ULL;
}

void pmm_free_page(uint64_t phys_addr) {
    if ((phys_addr & (PMM_PAGE_SIZE - 1ULL)) != 0ULL) {
        return;
    }

    const uint64_t page = phys_addr / PMM_PAGE_SIZE;
    if (!pmm_page_valid(page)) {
        return;
    }

    pmm_bit_clear(page);
}

uint64_t pmm_total_pages(void) {
    return g_total_pages;
}

uint64_t pmm_free_pages(void) {
    return g_free_pages;
}
