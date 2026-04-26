#ifndef AURORA_BOOT_MULTIBOOT2_H
#define AURORA_BOOT_MULTIBOOT2_H

#include <aurora/core/types.h>

enum {
    MULTIBOOT2_TAG_TYPE_END = 0U,
    MULTIBOOT2_TAG_TYPE_MMAP = 6U
};

typedef struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) multiboot_tag_t;

typedef struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed)) multiboot_mmap_entry_t;

typedef struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_mmap_entry_t entries[];
} __attribute__((packed)) multiboot_tag_mmap_t;

#endif
