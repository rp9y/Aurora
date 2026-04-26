#include <aurora/arch/x86_64/cpu.h>
#include <aurora/arch/x86_64/cpu_ops.h>
#include <aurora/core/panic.h>
#include <aurora/core/printk.h>

__attribute__((noreturn)) void panic_halt(const char *reason) {
    panic_halt_code(reason, 0ULL, 0ULL);
}

__attribute__((noreturn)) void panic_halt_code(const char *reason, uint64_t code, uint64_t context) {
    const char *message = (reason != (const char *)0) ? reason : "unknown";
    printk("[aurora] panic: %s code=0x%X context=0x%X\n", message, code, context);

    cpu_halt_forever();
}
