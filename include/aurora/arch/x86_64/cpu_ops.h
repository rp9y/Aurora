#ifndef AURORA_ARCH_X86_64_CPU_OPS_H
#define AURORA_ARCH_X86_64_CPU_OPS_H

#include <aurora/core/types.h>

void cpu_enable_interrupts(void);
void cpu_disable_interrupts(void);
void cpu_halt_once(void);
__attribute__((noreturn)) void cpu_halt_forever(void);
uint64_t cpu_read_cr2_value(void);
uint64_t cpu_read_cr3_value(void);
void cpu_write_cr3_value(uint64_t value);

#endif
