#include "core/syscall.h"

#include "core/debug/log.h"
#include "core/memory.h"
#include "core/scheduler.h"

enum {
    MAX_SYSCALLS = 256,
    SYSCALL_PING = 0,
    SYSCALL_UPTIME_TICKS = 1,
    SYSCALL_TASK_COUNT = 2,
    SYSCALL_ALLOC = 3,
};

static syscall_handler_t g_syscalls[MAX_SYSCALLS];
static uint64_t g_ticks;

static uint64_t syscall_ping(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1;
    (void)a2;
    (void)a3;
    return a0 ^ 0xA0F0ULL;
}

static uint64_t syscall_uptime(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    return g_ticks;
}

static uint64_t syscall_task_count_handler(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    return scheduler_task_count();
}

static uint64_t syscall_alloc_handler(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1;
    (void)a2;
    (void)a3;
    return (uint64_t)(uintptr_t)memory_alloc((size_t)a0);
}

void syscall_init(void) {
    for (size_t i = 0; i < MAX_SYSCALLS; i++) {
        g_syscalls[i] = 0;
    }
    g_ticks = 0;
    syscall_register(SYSCALL_PING, syscall_ping);
    syscall_register(SYSCALL_UPTIME_TICKS, syscall_uptime);
    syscall_register(SYSCALL_TASK_COUNT, syscall_task_count_handler);
    syscall_register(SYSCALL_ALLOC, syscall_alloc_handler);
    log_write(LOG_LEVEL_INFO, "syscall: initialized");
}

bool syscall_register(uint64_t id, syscall_handler_t handler) {
    if (id >= MAX_SYSCALLS) {
        return false;
    }
    g_syscalls[id] = handler;
    return true;
}

uint64_t syscall_dispatch(uint64_t id, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    if (id >= MAX_SYSCALLS || g_syscalls[id] == 0) {
        return (uint64_t)-1;
    }
    return g_syscalls[id](a0, a1, a2, a3);
}

void syscall_notify_tick(void) {
    g_ticks++;
}
