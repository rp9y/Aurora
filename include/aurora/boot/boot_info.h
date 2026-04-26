#ifndef AURORA_BOOT_BOOT_INFO_H
#define AURORA_BOOT_BOOT_INFO_H

#include <aurora/core/types.h>

typedef struct boot_memory_region {
    uint64_t start;
    uint64_t length;
    uint32_t type;
} boot_memory_region_t;

void boot_info_init(uintptr_t multiboot_info_addr);
uint32_t boot_info_total_size(void);
size_t boot_info_memory_region_count(void);
bool boot_info_memory_region_at(size_t index, boot_memory_region_t *out_region);

#endif
