#include "aurora/boot/boot_info.h"

static boot_info_t g_boot_info;

void boot_info_set(uint32_t boot_magic, uint64_t boot_info_addr) {
    g_boot_info.boot_magic = boot_magic;
    g_boot_info.boot_info_addr = boot_info_addr;
    g_boot_info.multiboot2_valid = (boot_magic == 0x36D76289u);
}

boot_info_t boot_info_get(void) {
    return g_boot_info;
}
