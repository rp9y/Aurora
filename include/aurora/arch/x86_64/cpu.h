#ifndef AURORA_ARCH_X86_64_CPU_H
#define AURORA_ARCH_X86_64_CPU_H

#include <stdbool.h>
#include <stdint.h>

void cpu_cli(void);
void cpu_sti(void);
void cpu_hlt(void);
void cpu_pause(void);

uint64_t cpu_read_cr0(void);
uint64_t cpu_read_cr2(void);
uint64_t cpu_read_cr3(void);
uint64_t cpu_read_cr4(void);
void cpu_write_cr0(uint64_t value);
void cpu_write_cr3(uint64_t value);
void cpu_write_cr4(uint64_t value);

uint64_t cpu_read_rflags(void);
bool cpu_interrupts_enabled(void);

#endif
