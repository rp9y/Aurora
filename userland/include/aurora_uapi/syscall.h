#ifndef AURORA_UAPI_SYSCALL_H
#define AURORA_UAPI_SYSCALL_H

#include <aurora/uapi/syscall_numbers.h>
#include <stdint.h>

static inline uint64_t sys_call6(
    uint64_t number,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5
) {
    register uint64_t rax __asm__("rax") = number;
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

static inline uint64_t sys_write_serial(const char *text, uint64_t length) {
    return sys_call6(SYSCALL_WRITE_SERIAL, (uint64_t)(uintptr_t)text, length, 0U, 0U, 0U, 0U);
}

static inline uint64_t sys_get_ticks(void) {
    return sys_call6(SYSCALL_GET_TICKS, 0U, 0U, 0U, 0U, 0U, 0U);
}

static inline int64_t sys_getchar(void) {
    return (int64_t)sys_call6(SYSCALL_GETCHAR, 0U, 0U, 0U, 0U, 0U, 0U);
}

static inline void sys_sleep(uint64_t ticks) {
    (void)sys_call6(SYSCALL_SLEEP, ticks, 0U, 0U, 0U, 0U, 0U);
}

#endif
