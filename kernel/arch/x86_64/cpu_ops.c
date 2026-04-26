#include <aurora/arch/x86_64/cpu_ops.h>

void cpu_enable_interrupts(void) {
    __asm__ volatile("sti" : : : "memory");
}

void cpu_disable_interrupts(void) {
    __asm__ volatile("cli" : : : "memory");
}

void cpu_halt_once(void) {
    __asm__ volatile("hlt");
}

__attribute__((noreturn)) void cpu_halt_forever(void) {
    cpu_disable_interrupts();
    for (;;) {
        cpu_halt_once();
    }
}

uint64_t cpu_read_cr2_value(void) {
    uint64_t value = 0ULL;
    __asm__ volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}

uint64_t cpu_read_cr3_value(void) {
    uint64_t value = 0ULL;
    __asm__ volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

void cpu_write_cr3_value(uint64_t value) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(value) : "memory");
}
