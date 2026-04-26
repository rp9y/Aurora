#include <aurora/drivers/serial.h>
#include <aurora/drivers/keyboard.h>
#include <aurora/mm/kheap.h>
#include <aurora/mm/pmm.h>
#include <aurora/sched/scheduler.h>
#include <aurora/sys/syscall.h>

enum {
    SYSCALL_TABLE_SIZE = 64U
};

static syscall_handler_t g_syscalls[SYSCALL_TABLE_SIZE];

static uint64_t sys_write_serial(uint64_t buffer_addr, uint64_t length, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;

    if (buffer_addr == 0ULL) {
        return (uint64_t)-1LL;
    }

    const char *buffer = (const char *)(uintptr_t)buffer_addr;
    serial_write_n(buffer, (size_t)length);
    return length;
}

static uint64_t sys_get_ticks(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    return scheduler_ticks();
}

static uint64_t sys_yield(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    scheduler_request_reschedule();
    return 0ULL;
}

static uint64_t sys_kmalloc(uint64_t size, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;

    void *ptr = kmalloc((size_t)size);
    return (uint64_t)(uintptr_t)ptr;
}

static uint64_t sys_kfree(uint64_t pointer, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;

    kfree((void *)(uintptr_t)pointer);
    return 0ULL;
}

static uint64_t sys_heap_used(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    return kheap_bytes_used();
}

static uint64_t sys_heap_total(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    return kheap_bytes_total();
}

static uint64_t sys_pmm_free_pages(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    return pmm_free_pages();
}

static uint64_t sys_getchar(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;

    return (uint64_t)(int64_t)keyboard_getchar();
}

static uint64_t sys_sleep(uint64_t ticks, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;

    scheduler_sleep(ticks);
    return 0ULL;
}

static uint64_t sys_invalid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    return (uint64_t)-1LL;
}

bool syscall_register(uint64_t syscall_number, syscall_handler_t handler) {
    if (syscall_number >= SYSCALL_TABLE_SIZE || handler == (syscall_handler_t)0) {
        return false;
    }

    g_syscalls[syscall_number] = handler;
    return true;
}

void syscall_init(void) {
    for (uint64_t i = 0U; i < SYSCALL_TABLE_SIZE; ++i) {
        g_syscalls[i] = sys_invalid;
    }

    (void)syscall_register(SYSCALL_WRITE_SERIAL, sys_write_serial);
    (void)syscall_register(SYSCALL_GET_TICKS, sys_get_ticks);
    (void)syscall_register(SYSCALL_YIELD, sys_yield);
    (void)syscall_register(SYSCALL_KMALLOC, sys_kmalloc);
    (void)syscall_register(SYSCALL_KFREE, sys_kfree);
    (void)syscall_register(SYSCALL_HEAP_USED, sys_heap_used);
    (void)syscall_register(SYSCALL_HEAP_TOTAL, sys_heap_total);
    (void)syscall_register(SYSCALL_PMM_FREE_PAGES, sys_pmm_free_pages);
    (void)syscall_register(SYSCALL_GETCHAR, sys_getchar);
    (void)syscall_register(SYSCALL_SLEEP, sys_sleep);

    serial_write("[aurora] syscall vector int 0x80 ready\n");
}

uint64_t syscall_dispatch(isr_frame_t *frame) {
    const uint64_t syscall_number = frame->rax;
    syscall_handler_t fn = sys_invalid;

    if (syscall_number < SYSCALL_TABLE_SIZE) {
        fn = g_syscalls[syscall_number];
    }

    return fn(frame->rdi, frame->rsi, frame->rdx, frame->r10, frame->r8, frame->r9);
}
