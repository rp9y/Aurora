#include <aurora/arch/x86_64/idt.h>
#include <aurora/lib/string.h>

typedef struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed)) idt_entry_t;

typedef struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

enum {
    IDT_ENTRIES = 256U,
    KERNEL_CODE_SELECTOR = 0x08U,
    IDT_GATE_INTERRUPT_RING0 = 0x8EU,
    IDT_GATE_INTERRUPT_RING3 = 0xEEU
};

extern void *isr_stub_table[];

static idt_entry_t g_idt[IDT_ENTRIES];

void idt_set_gate(uint8_t vector, uintptr_t handler, uint8_t type_attr) {
    idt_entry_t *entry = &g_idt[vector];

    entry->offset_low = (uint16_t)(handler & 0xFFFFU);
    entry->selector = KERNEL_CODE_SELECTOR;
    entry->ist = 0U;
    entry->type_attr = type_attr;
    entry->offset_mid = (uint16_t)((handler >> 16U) & 0xFFFFU);
    entry->offset_high = (uint32_t)((handler >> 32U) & 0xFFFFFFFFU);
    entry->zero = 0U;
}

void idt_init(void) {
    static const uint8_t vectors[] = {
        0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U,
        8U, 9U, 10U, 11U, 12U, 13U, 14U, 15U,
        16U, 17U, 18U, 19U, 20U, 21U, 22U, 23U,
        24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U,
        32U, 33U, 34U, 35U, 36U, 37U, 38U, 39U,
        40U, 41U, 42U, 43U, 44U, 45U, 46U, 47U,
        128U
    };

    aurora_memset(g_idt, 0, sizeof(g_idt));

    for (size_t i = 0U; i < (sizeof(vectors) / sizeof(vectors[0])); ++i) {
        const uint8_t vector = vectors[i];
        const uint8_t attr = (vector == 128U) ? IDT_GATE_INTERRUPT_RING3 : IDT_GATE_INTERRUPT_RING0;
        idt_set_gate(vector, (uintptr_t)isr_stub_table[i], attr);
    }

    const idtr_t idtr = {
        .limit = (uint16_t)(sizeof(g_idt) - 1U),
        .base = (uint64_t)(uintptr_t)&g_idt[0]
    };

    __asm__ volatile("lidt %0" : : "m"(idtr));
}
