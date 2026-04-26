#ifndef AURORA_ARCH_X86_64_CPUID_H
#define AURORA_ARCH_X86_64_CPUID_H

#include <aurora/core/types.h>

typedef struct cpu_features {
    char vendor[13];
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    bool has_tsc;
    bool has_apic;
    bool has_x2apic;
    bool has_syscall;
    bool has_nx;
} cpu_features_t;

void cpu_detect_features(cpu_features_t *features);

#endif
