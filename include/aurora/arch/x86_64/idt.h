#ifndef AURORA_ARCH_X86_64_IDT_H
#define AURORA_ARCH_X86_64_IDT_H

#include <stdint.h>

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

void idt_set_gate(uint8_t vector, uintptr_t handler, uint8_t flags);
void idt_init(void);
void idt_load(void);

#endif
