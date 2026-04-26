#ifndef AURORA_CORE_PANIC_H
#define AURORA_CORE_PANIC_H

#include <aurora/core/types.h>

__attribute__((noreturn)) void panic_halt(const char *reason);
__attribute__((noreturn)) void panic_halt_code(const char *reason, uint64_t code, uint64_t context);

#endif
