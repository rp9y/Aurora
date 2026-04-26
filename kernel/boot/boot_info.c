#include <aurora/boot/boot_info.h>
#include <aurora/boot/multiboot2.h>
#include <aurora/lib/string.h>

enum {
    BOOT_MEMORY_REGIONS_MAX = 128U
};

static uint32_t g_total_size = 0U;
static size_t g_region_count = 0U;
static boot_memory_region_t g_regions[BOOT_MEMORY_REGIONS_MAX];

void boot_info_init(uintptr_t multiboot_info_addr) {
    if (multiboot_info_addr == 0U) {
        g_total_size = 0U;
        g_region_count = 0U;
        aurora_memset(g_regions, 0, sizeof(g_regions));
        return;
    }

    g_total_size = *(const uint32_t *)(uintptr_t)multiboot_info_addr;
    g_region_count = 0U;
    aurora_memset(g_regions, 0, sizeof(g_regions));

    const uintptr_t tags_start = multiboot_info_addr + 8U;
    const uintptr_t tags_end = multiboot_info_addr + (uintptr_t)g_total_size;

    for (uintptr_t cursor = tags_start; cursor < tags_end;) {
        const multiboot_tag_t *tag = (const multiboot_tag_t *)cursor;
        if (tag->type == MULTIBOOT2_TAG_TYPE_END) {
            break;
        }

        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            const multiboot_tag_mmap_t *mmap_tag = (const multiboot_tag_mmap_t *)tag;
            const uint32_t entry_size = mmap_tag->entry_size;
            uintptr_t entry_cursor = (uintptr_t)&mmap_tag->entries[0];
            const uintptr_t entry_end = cursor + mmap_tag->size;

            while (entry_cursor + entry_size <= entry_end && g_region_count < BOOT_MEMORY_REGIONS_MAX) {
                const multiboot_mmap_entry_t *entry = (const multiboot_mmap_entry_t *)entry_cursor;
                g_regions[g_region_count].start = entry->addr;
                g_regions[g_region_count].length = entry->len;
                g_regions[g_region_count].type = entry->type;
                ++g_region_count;
                entry_cursor += entry_size;
            }
        }

        cursor += (uintptr_t)((tag->size + 7U) & ~7U);
    }
}

uint32_t boot_info_total_size(void) {
    return g_total_size;
}

size_t boot_info_memory_region_count(void) {
    return g_region_count;
}

bool boot_info_memory_region_at(size_t index, boot_memory_region_t *out_region) {
    if (out_region == (boot_memory_region_t *)0 || index >= g_region_count) {
        return false;
    }

    *out_region = g_regions[index];
    return true;
}
