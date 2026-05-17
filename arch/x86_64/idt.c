#include "aurora/arch/x86_64/idt.h"

#include "aurora/arch/x86_64/isr.h"

extern void* isr_stub_table[48];

static idt_entry_t g_idt[256];
static idtr_t g_idtr;

void idt_set_gate(uint8_t vector, uintptr_t handler, uint8_t flags) {
    idt_entry_t* entry = &g_idt[vector];
    entry->offset_low = (uint16_t)(handler & 0xFFFFu);
    entry->selector = 0x08u;
    entry->ist = 0u;
    entry->type_attr = flags;
    entry->offset_mid = (uint16_t)((handler >> 16) & 0xFFFFu);
    entry->offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFFu);
    entry->reserved = 0u;
}

void idt_load(void) {
    __asm__ volatile("lidt %0" : : "m"(g_idtr));
}

void idt_init(void) {
    const uintptr_t default_stub = (uintptr_t)isr_stub_table[47];
    for (size_t i = 0; i < 256; i++) {
        idt_set_gate((uint8_t)i, default_stub, 0x8E);
    }

    for (size_t i = 0; i < 48; i++) {
        idt_set_gate((uint8_t)i, (uintptr_t)isr_stub_table[i], 0x8E);
    }

    g_idtr.limit = (uint16_t)(sizeof(g_idt) - 1u);
    g_idtr.base = (uint64_t)(uintptr_t)&g_idt[0];
    idt_load();

    isr_init();
}
