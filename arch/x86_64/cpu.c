#include "aurora/arch/x86_64/cpu.h"

void cpu_cli(void) {
    __asm__ volatile("cli");
}

void cpu_sti(void) {
    __asm__ volatile("sti");
}

void cpu_hlt(void) {
    __asm__ volatile("hlt");
}

void cpu_pause(void) {
    __asm__ volatile("pause");
}

uint64_t cpu_read_cr0(void) {
    uint64_t value;
    __asm__ volatile("mov %%cr0, %0" : "=r"(value));
    return value;
}

uint64_t cpu_read_cr2(void) {
    uint64_t value;
    __asm__ volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}

uint64_t cpu_read_cr3(void) {
    uint64_t value;
    __asm__ volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

uint64_t cpu_read_cr4(void) {
    uint64_t value;
    __asm__ volatile("mov %%cr4, %0" : "=r"(value));
    return value;
}

void cpu_write_cr0(uint64_t value) {
    __asm__ volatile("mov %0, %%cr0" : : "r"(value) : "memory");
}

void cpu_write_cr3(uint64_t value) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(value) : "memory");
}

void cpu_write_cr4(uint64_t value) {
    __asm__ volatile("mov %0, %%cr4" : : "r"(value) : "memory");
}

uint64_t cpu_read_rflags(void) {
    uint64_t value;
    __asm__ volatile("pushfq; popq %0" : "=r"(value));
    return value;
}

bool cpu_interrupts_enabled(void) {
    return (cpu_read_rflags() & (1ULL << 9)) != 0;
}
