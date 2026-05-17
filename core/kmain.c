#include "core/kmain.h"

#include "aurora/arch/x86_64/cpu.h"
#include "aurora/boot/boot_info.h"
#include "core/console.h"
#include "core/debug/log.h"
#include "core/init.h"
#include "core/interrupt.h"

void kmain(uint32_t boot_magic, uint64_t boot_info_addr) {
    boot_info_set(boot_magic, boot_info_addr);
    system_init();

    boot_info_t info = boot_info_get();
    log_printf(LOG_LEVEL_INFO, "boot: magic=0x%x info=0x%x mb2=%u",
               (uint64_t)info.boot_magic,
               info.boot_info_addr,
               info.multiboot2_valid ? 1ULL : 0ULL);

    interrupt_enable();
    console_run_scripted_boot_demo();

    for (;;) {
        console_poll();
        cpu_hlt();
    }
}
