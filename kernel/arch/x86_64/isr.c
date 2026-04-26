#include <aurora/arch/x86_64/cpu.h>
#include <aurora/arch/x86_64/cpu_ops.h>
#include <aurora/arch/x86_64/isr.h>
#include <aurora/arch/x86_64/pic.h>
#include <aurora/core/panic.h>
#include <aurora/drivers/pit.h>
#include <aurora/drivers/serial.h>
#include <aurora/sched/scheduler.h>
#include <aurora/sys/syscall.h>

static interrupt_handler_t g_handlers[256];

static void panic_exception(isr_frame_t *frame) {
    static const char *names[32] = {
        "Divide Error", "Debug", "NMI", "Breakpoint", "Overflow", "Bound Range",
        "Invalid Opcode", "Device Not Available", "Double Fault", "Coprocessor Segment Overrun",
        "Invalid TSS", "Segment Not Present", "Stack-Segment Fault", "General Protection Fault",
        "Page Fault", "Reserved", "x87 Floating-Point", "Alignment Check", "Machine Check",
        "SIMD Floating-Point", "Virtualization", "Control Protection", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved", "Hypervisor Injection",
        "VMM Communication", "Security", "Reserved"
    };

    const uint64_t vector = frame->vector;
    const char *name = (vector < 32U) ? names[vector] : "Unknown";

    serial_write("[aurora] exception: ");
    serial_write(name);
    serial_write(" (vector=");
    serial_write_dec_u64(vector);
    serial_write(" error=");
    serial_write_hex_u64(frame->error_code);
    serial_write(" rip=");
    serial_write_hex_u64(frame->rip);
    serial_write(")\n");

    if (vector == 14U) {
        serial_write("[aurora] page fault address=");
        serial_write_hex_u64(cpu_read_cr2_value());
        serial_write_char('\n');
    }

    panic_halt_code("exception", frame->vector, frame->rip);
}

void isr_register_handler(uint8_t vector, interrupt_handler_t handler) {
    g_handlers[vector] = handler;
}

void isr_unregister_handler(uint8_t vector) {
    g_handlers[vector] = (interrupt_handler_t)0;
}

void isr_dispatch(isr_frame_t *frame) {
    const uint8_t vector = (uint8_t)(frame->vector & 0xFFU);
    interrupt_handler_t handler = g_handlers[vector];

    if (vector == 32U) {
        pit_handle_irq();
        scheduler_tick();
        pic_send_eoi(0U);
    } else if (vector >= 32U && vector <= 47U) {
        pic_send_eoi((uint8_t)(vector - 32U));
    } else if (vector == 128U) {
        frame->rax = syscall_dispatch(frame);
        return;
    }

    if (handler != (interrupt_handler_t)0) {
        handler(frame);
        return;
    }

    if (vector < 32U) {
        panic_exception(frame);
    }
}
