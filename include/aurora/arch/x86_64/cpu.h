#ifndef AURORA_ARCH_X86_64_CPU_H
#define AURORA_ARCH_X86_64_CPU_H

#include <aurora/core/types.h>

static inline void cpu_cli(void) {
    __asm__ volatile("cli" : : : "memory");
}

static inline void cpu_sti(void) {
    __asm__ volatile("sti" : : : "memory");
}

static inline void cpu_hlt(void) {
    __asm__ volatile("hlt");
}

static inline uint64_t cpu_read_rflags(void) {
    uint64_t value = 0U;
    __asm__ volatile("pushfq; popq %0" : "=r"(value));
    return value;
}

static inline void cpu_pause(void) {
    __asm__ volatile("pause");
}

#endif
