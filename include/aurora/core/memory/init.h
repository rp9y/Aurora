#ifndef AURORA_CORE_MEMORY_INIT_H
#define AURORA_CORE_MEMORY_INIT_H

#include <aurora/core/types.h>

void core_memory_init(uintptr_t multiboot_info_addr, uintptr_t kernel_start, uintptr_t kernel_end);
void core_memory_log_stats(void);

#endif
