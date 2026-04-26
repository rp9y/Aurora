#ifndef AURORA_SYS_INT80_H
#define AURORA_SYS_INT80_H

#include <aurora/core/types.h>

static inline uint64_t int80_call6(
    uint64_t syscall_number,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5
) {
    register uint64_t rax __asm__("rax") = syscall_number;
    register uint64_t rdi __asm__("rdi") = arg0;
    register uint64_t rsi __asm__("rsi") = arg1;
    register uint64_t rdx __asm__("rdx") = arg2;
    register uint64_t r10 __asm__("r10") = arg3;
    register uint64_t r8 __asm__("r8") = arg4;
    register uint64_t r9 __asm__("r9") = arg5;

    __asm__ volatile(
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8), "r"(r9)
        : "cc", "memory", "rcx", "r11"
    );

    return rax;
}

static inline uint64_t int80_call3(uint64_t syscall_number, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    return int80_call6(syscall_number, arg0, arg1, arg2, 0ULL, 0ULL, 0ULL);
}

static inline uint64_t int80_call1(uint64_t syscall_number, uint64_t arg0) {
    return int80_call6(syscall_number, arg0, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
}

static inline uint64_t int80_call0(uint64_t syscall_number) {
    return int80_call6(syscall_number, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
}

#endif
