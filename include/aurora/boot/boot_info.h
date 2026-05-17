#ifndef AURORA_BOOT_BOOT_INFO_H
#define AURORA_BOOT_BOOT_INFO_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t boot_magic;
    uint64_t boot_info_addr;
    bool multiboot2_valid;
} boot_info_t;

void boot_info_set(uint32_t boot_magic, uint64_t boot_info_addr);
boot_info_t boot_info_get(void);

#endif
