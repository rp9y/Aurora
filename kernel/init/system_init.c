#include <aurora/core/debug/init.h>
#include <aurora/core/init.h>
#include <aurora/core/init/stage.h>
#include <aurora/core/interrupt/init.h>
#include <aurora/core/ipc/init.h>
#include <aurora/core/libk/init.h>
#include <aurora/core/memory/init.h>
#include <aurora/core/scheduler/init.h>
#include <aurora/core/syscall/init.h>

void core_system_init(
    uintptr_t multiboot_info_addr,
    uintptr_t kernel_start,
    uintptr_t kernel_end,
    uint32_t pit_frequency_hz,
    bool enable_keyboard
) {
    core_init_stage_early();
    core_libk_init();
    core_debug_init();
    core_interrupt_init(pit_frequency_hz, enable_keyboard);
    core_memory_init(multiboot_info_addr, kernel_start, kernel_end);
    core_init_stage_post_memory();
    core_ipc_init();
    core_scheduler_init();
    core_init_stage_post_scheduler();
    core_syscall_init();
}
