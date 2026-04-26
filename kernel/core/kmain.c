#include <aurora/arch/x86_64/cpu.h>
#include <aurora/arch/x86_64/cpuid.h>
#include <aurora/core/debug/init.h>
#include <aurora/core/console.h>
#include <aurora/core/init.h>
#include <aurora/core/interrupt/init.h>
#include <aurora/core/kernel.h>
#include <aurora/core/libk/init.h>
#include <aurora/core/memory/init.h>
#include <aurora/core/panic.h>
#include <aurora/core/printk.h>
#include <aurora/core/scheduler/init.h>
#include <aurora/core/syscall/init.h>
#include <aurora/drivers/serial.h>
#include <aurora/drivers/vga_text.h>
#include <aurora/mm/kheap.h>
#include <aurora/mm/paging.h>
#include <aurora/mm/vmm.h>
#include <aurora/sched/scheduler.h>
#include <aurora/sys/int80.h>
#include <aurora/sys/syscall.h>

enum {
    MULTIBOOT2_BOOTLOADER_MAGIC = 0x36D76289U
};

static void print_boot_header(uint32_t multiboot_magic, uint32_t multiboot_info) {
    printk("[aurora] serial: online\n");
    printk("[aurora] boot magic: 0x%X\n", (uint64_t)multiboot_magic);
    printk("[aurora] multiboot info: 0x%X\n", (uint64_t)multiboot_info);
}

static void print_cpu_features(void) {
    cpu_features_t features;
    cpu_detect_features(&features);

    printk(
        "[aurora] cpu vendor=%s family=%u model=%u stepping=%u\n",
        features.vendor,
        (uint64_t)features.family,
        (uint64_t)features.model,
        (uint64_t)features.stepping
    );
    printk(
        "[aurora] cpu feat tsc=%u apic=%u x2apic=%u syscall=%u nx=%u\n",
        features.has_tsc ? 1ULL : 0ULL,
        features.has_apic ? 1ULL : 0ULL,
        features.has_x2apic ? 1ULL : 0ULL,
        features.has_syscall ? 1ULL : 0ULL,
        features.has_nx ? 1ULL : 0ULL
    );
}

static void worker_task(void *arg) {
    (void)arg;

    for (;;) {
        void *probe = kmalloc(96U);
        if (probe != (void *)0) {
            kfree(probe);
        }
        scheduler_sleep(25U);
    }
}

static void syscall_task(void *arg) {
    (void)arg;

    for (;;) {
        (void)int80_call0(SYSCALL_GET_TICKS);
        void *ptr = (void *)(uintptr_t)int80_call1(SYSCALL_KMALLOC, 128U);
        if (ptr != (void *)0) {
            (void)int80_call1(SYSCALL_KFREE, (uint64_t)(uintptr_t)ptr);
        }
        (void)int80_call1(SYSCALL_SLEEP, 25U);
    }
}

static void allocator_task(void *arg) {
    (void)arg;

    void *buffer = (void *)0;
    void *vm_region = (void *)0;
    size_t size = 64U;

    for (;;) {
        if (buffer == (void *)0) {
            buffer = kmalloc(size);
        } else {
            buffer = krealloc(buffer, size);
        }

        if (buffer != (void *)0) {
            size += 64U;
            if (size > 2048U) {
                size = 64U;
            }
        }

        if (vm_region == (void *)0) {
            vm_region = vmm_alloc(8192U, PAGE_WRITABLE | PAGE_NO_EXECUTE);
        } else {
            vmm_free(vm_region, 8192U);
            vm_region = (void *)0;
        }

        scheduler_sleep(50U);
    }
}

static void console_task(void *arg) {
    (void)arg;
    console_run();
}

void kmain(uint32_t multiboot_magic, uint32_t multiboot_info) {
    extern uint8_t __kernel_start;
    extern uint8_t __kernel_end;

    cpu_cli();
    vga_text_init();

    const bool serial_ok = serial_init();
    if (!serial_ok) {
        panic_halt("serial init failed");
    }

    printk("\n");
    printk("[aurora] kernel entry reached\n");
    print_boot_header(multiboot_magic, multiboot_info);
    print_cpu_features();

    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        printk("[aurora] warning: multiboot2 magic missing, using fallback memory profile\n");
    }

    core_system_init(
        (uintptr_t)multiboot_info,
        (uintptr_t)&__kernel_start,
        (uintptr_t)&__kernel_end,
        100U,
        true
    );
    console_init();
    core_memory_log_stats();

    (void)core_scheduler_spawn("worker", worker_task, (void *)0);
    (void)core_scheduler_spawn("syscall", syscall_task, (void *)0);
    (void)core_scheduler_spawn("allocator", allocator_task, (void *)0);
    (void)core_scheduler_spawn("console", console_task, (void *)0);

    printk("[aurora] interrupts enabled\n");
    cpu_sti();

    printk("[aurora] scheduler online\n");

    for (;;) {
        scheduler_maybe_preempt();
        core_debug_flush();
        cpu_hlt();
    }
}
