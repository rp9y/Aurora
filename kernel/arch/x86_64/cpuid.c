#include <aurora/arch/x86_64/cpuid.h>
#include <aurora/lib/string.h>

static void cpuid_query(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    uint32_t ra = 0U;
    uint32_t rb = 0U;
    uint32_t rc = 0U;
    uint32_t rd = 0U;

    __asm__ volatile(
        "cpuid"
        : "=a"(ra), "=b"(rb), "=c"(rc), "=d"(rd)
        : "a"(leaf), "c"(subleaf)
    );

    *eax = ra;
    *ebx = rb;
    *ecx = rc;
    *edx = rd;
}

void cpu_detect_features(cpu_features_t *features) {
    if (features == (cpu_features_t *)0) {
        return;
    }

    aurora_memset(features, 0, sizeof(*features));

    uint32_t eax = 0U;
    uint32_t ebx = 0U;
    uint32_t ecx = 0U;
    uint32_t edx = 0U;

    cpuid_query(0U, 0U, &eax, &ebx, &ecx, &edx);
    const uint32_t max_basic = eax;

    aurora_memcpy(&features->vendor[0], &ebx, sizeof(ebx));
    aurora_memcpy(&features->vendor[4], &edx, sizeof(edx));
    aurora_memcpy(&features->vendor[8], &ecx, sizeof(ecx));
    features->vendor[12] = '\0';

    if (max_basic >= 1U) {
        cpuid_query(1U, 0U, &eax, &ebx, &ecx, &edx);

        const uint32_t base_family = (eax >> 8U) & 0x0FU;
        const uint32_t ext_family = (eax >> 20U) & 0xFFU;
        const uint32_t base_model = (eax >> 4U) & 0x0FU;
        const uint32_t ext_model = (eax >> 16U) & 0x0FU;

        features->stepping = eax & 0x0FU;
        features->family = (base_family == 0x0FU) ? (base_family + ext_family) : base_family;
        features->model = (base_family == 0x06U || base_family == 0x0FU) ? ((ext_model << 4U) | base_model) : base_model;

        features->has_tsc = ((edx >> 4U) & 1U) != 0U;
        features->has_apic = ((edx >> 9U) & 1U) != 0U;
        features->has_x2apic = ((ecx >> 21U) & 1U) != 0U;
    }

    cpuid_query(0x80000000U, 0U, &eax, &ebx, &ecx, &edx);
    const uint32_t max_extended = eax;
    if (max_extended >= 0x80000001U) {
        cpuid_query(0x80000001U, 0U, &eax, &ebx, &ecx, &edx);
        features->has_syscall = ((edx >> 11U) & 1U) != 0U;
        features->has_nx = ((edx >> 20U) & 1U) != 0U;
    }
}
