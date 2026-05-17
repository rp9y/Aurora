#ifndef AURORA_ARCH_X86_64_ISR_H
#define AURORA_ARCH_X86_64_ISR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
} isr_frame_t;

typedef void (*isr_handler_t)(isr_frame_t* frame);

void isr_init(void);
void isr_register(uint8_t vector, isr_handler_t handler);
void isr_unregister(uint8_t vector);
bool isr_has_handler(uint8_t vector);
void isr_dispatch(isr_frame_t* frame);

#endif
