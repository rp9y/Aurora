#ifndef AURORA_CORE_INIT_H
#define AURORA_CORE_INIT_H

#include <aurora/core/types.h>

void core_system_init(
    uintptr_t multiboot_info_addr,
    uintptr_t kernel_start,
    uintptr_t kernel_end,
    uint32_t pit_frequency_hz,
    bool enable_keyboard
);

#endif
