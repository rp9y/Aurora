#ifndef AURORA_SYS_SYSCALL_H
#define AURORA_SYS_SYSCALL_H

#include <aurora/arch/x86_64/isr.h>
#include <aurora/core/types.h>
#include <aurora/uapi/syscall_numbers.h>

typedef uint64_t (*syscall_handler_t)(
    uint64_t a0,
    uint64_t a1,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5
);

void syscall_init(void);
bool syscall_register(uint64_t syscall_number, syscall_handler_t handler);
uint64_t syscall_dispatch(isr_frame_t *frame);

#endif
