#include "core/panic.h"

#include "aurora/arch/x86_64/cpu.h"
#include "core/debug/log.h"
#include "core/libk/format.h"

void panic(const char* message) {
    log_printf(LOG_LEVEL_ERROR, "PANIC: %s", message);
    cpu_cli();
    for (;;) {
        cpu_hlt();
    }
}

void panicf(const char* fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    panic(buffer);
}
