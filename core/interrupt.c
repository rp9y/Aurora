#include "core/interrupt.h"

#include "aurora/arch/x86_64/cpu.h"
#include "aurora/arch/x86_64/idt.h"
#include "aurora/arch/x86_64/io.h"
#include "aurora/arch/x86_64/isr.h"
#include "aurora/arch/x86_64/pic.h"
#include "core/debug/log.h"
#include "core/scheduler.h"
#include "core/syscall.h"

static volatile uint64_t g_ticks;

static void irq_timer_handler(isr_frame_t* frame) {
    (void)frame;
    g_ticks++;
    syscall_notify_tick();
    scheduler_tick(g_ticks);
}

static void irq_keyboard_handler(isr_frame_t* frame) {
    (void)frame;
    const uint8_t scan_code = io_in8(0x60);
    log_printf(LOG_LEVEL_DEBUG, "kbd: scan=0x%x", (uint64_t)scan_code);
}

void interrupt_init(void) {
    g_ticks = 0;

    idt_init();
    pic_remap(32, 40);

    isr_register(32, irq_timer_handler);
    isr_register(33, irq_keyboard_handler);

    pic_unmask_irq(0);
    pic_unmask_irq(1);

    log_write(LOG_LEVEL_INFO, "interrupts: IDT/PIC initialized");
}

void interrupt_enable(void) {
    cpu_sti();
}

void interrupt_disable(void) {
    cpu_cli();
}

uint64_t interrupt_ticks(void) {
    return g_ticks;
}
