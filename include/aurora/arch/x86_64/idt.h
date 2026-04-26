#ifndef AURORA_ARCH_X86_64_IDT_H
#define AURORA_ARCH_X86_64_IDT_H

#include <aurora/core/types.h>

void idt_init(void);
void idt_set_gate(uint8_t vector, uintptr_t handler, uint8_t type_attr);

#endif
