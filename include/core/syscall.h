#ifndef CORE_SYSCALL_H
#define CORE_SYSCALL_H

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t (*syscall_handler_t)(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3);

void syscall_init(void);
bool syscall_register(uint64_t id, syscall_handler_t handler);
uint64_t syscall_dispatch(uint64_t id, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3);
void syscall_notify_tick(void);

#endif
