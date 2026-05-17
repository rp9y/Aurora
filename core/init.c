#include "core/init.h"

#include "aurora/arch/x86_64/paging.h"
#include "core/console.h"
#include "core/debug/log.h"
#include "core/debug/serial.h"
#include "core/interrupt.h"
#include "core/init_stage.h"
#include "core/memory.h"
#include "core/scheduler.h"
#include "core/syscall.h"

void system_init(void) {
    uint64_t mark = 0;

    init_stage_begin("serial");
    serial_init();
    log_set_level(LOG_LEVEL_INFO);
    log_write(LOG_LEVEL_INFO, "aurora: serial online");
    init_stage_end("serial", mark);

    mark = interrupt_ticks();
    init_stage_begin("paging");
    paging_init();
    init_stage_end("paging", mark);

    mark = interrupt_ticks();
    init_stage_begin("memory");
    memory_init();
    init_stage_end("memory", mark);

    mark = interrupt_ticks();
    init_stage_begin("scheduler");
    scheduler_init();
    init_stage_end("scheduler", mark);

    mark = interrupt_ticks();
    init_stage_begin("syscall");
    syscall_init();
    init_stage_end("syscall", mark);

    mark = interrupt_ticks();
    init_stage_begin("interrupt");
    interrupt_init();
    init_stage_end("interrupt", mark);

    scheduler_create_task("idle");
    scheduler_create_task("shell");
    scheduler_create_task("worker");

    console_init();
    log_write(LOG_LEVEL_INFO, "aurora: boot sequence complete");
}
