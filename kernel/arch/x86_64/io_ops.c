#include <aurora/arch/x86_64/io_ops.h>

void io_out8(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void io_out16(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

void io_out32(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t io_in8(uint16_t port) {
    uint8_t value = 0U;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

uint16_t io_in16(uint16_t port) {
    uint16_t value = 0U;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

uint32_t io_in32(uint16_t port) {
    uint32_t value = 0U;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void io_wait_cycle(void) {
    __asm__ volatile("outb %%al, $0x80" : : "a"(0U));
}
