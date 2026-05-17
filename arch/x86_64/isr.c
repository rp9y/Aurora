#include "aurora/arch/x86_64/isr.h"

#include "aurora/arch/x86_64/pic.h"
#include "core/debug/log.h"
#include "core/panic.h"

extern void* isr_stub_table[48];

static isr_handler_t g_handlers[256];

static const char* exception_name(uint8_t vector) {
    static const char* k_names[32] = {
        "Divide Error",
        "Debug",
        "NMI",
        "Breakpoint",
        "Overflow",
        "BOUND Range",
        "Invalid Opcode",
        "Device Not Available",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "Segment Not Present",
        "Stack-Segment Fault",
        "General Protection Fault",
        "Page Fault",
        "Reserved",
        "x87 Floating Point",
        "Alignment Check",
        "Machine Check",
        "SIMD Floating Point",
        "Virtualization",
        "Control Protection",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Hypervisor Injection",
        "VMM Communication",
        "Security",
        "Reserved",
    };
    if (vector < 32) {
        return k_names[vector];
    }
    return "Interrupt";
}

void isr_init(void) {
    for (size_t i = 0; i < 256; i++) {
        g_handlers[i] = 0;
    }
}

void isr_register(uint8_t vector, isr_handler_t handler) {
    g_handlers[vector] = handler;
}

void isr_unregister(uint8_t vector) {
    g_handlers[vector] = 0;
}

bool isr_has_handler(uint8_t vector) {
    return g_handlers[vector] != 0;
}

void isr_dispatch(isr_frame_t* frame) {
    const uint8_t vector = (uint8_t)frame->vector;
    isr_handler_t handler = g_handlers[vector];
    if (handler != 0) {
        handler(frame);
    } else if (vector < 32) {
        log_printf(LOG_LEVEL_ERROR, "Unhandled exception %u (%s), err=0x%llx rip=0x%llx",
                   (unsigned)vector, exception_name(vector),
                   (unsigned long long)frame->error_code,
                   (unsigned long long)frame->rip);
        panic("fatal CPU exception");
    }

    if (vector >= 32 && vector <= 47) {
        pic_send_eoi((uint8_t)(vector - 32));
    }
}
