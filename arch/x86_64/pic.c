#include "aurora/arch/x86_64/pic.h"

#include "aurora/arch/x86_64/io.h"

enum {
    ICW1_ICW4 = 0x01,
    ICW1_INIT = 0x10,
    ICW4_8086 = 0x01,
};

void pic_remap(uint8_t master_offset, uint8_t slave_offset) {
    const uint8_t master_mask = io_in8(PIC1_DATA);
    const uint8_t slave_mask = io_in8(PIC2_DATA);

    io_out8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    io_out8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    io_out8(PIC1_DATA, master_offset);
    io_wait();
    io_out8(PIC2_DATA, slave_offset);
    io_wait();

    io_out8(PIC1_DATA, 0x04);
    io_wait();
    io_out8(PIC2_DATA, 0x02);
    io_wait();

    io_out8(PIC1_DATA, ICW4_8086);
    io_wait();
    io_out8(PIC2_DATA, ICW4_8086);
    io_wait();

    io_out8(PIC1_DATA, master_mask);
    io_out8(PIC2_DATA, slave_mask);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        io_out8(PIC2_COMMAND, 0x20);
    }
    io_out8(PIC1_COMMAND, 0x20);
}

void pic_mask_irq(uint8_t irq) {
    const uint16_t mask = (uint16_t)(pic_get_mask() | (1u << irq));
    pic_set_mask(mask);
}

void pic_unmask_irq(uint8_t irq) {
    const uint16_t mask = (uint16_t)(pic_get_mask() & ~(1u << irq));
    pic_set_mask(mask);
}

uint16_t pic_get_mask(void) {
    const uint8_t master = io_in8(PIC1_DATA);
    const uint8_t slave = io_in8(PIC2_DATA);
    return (uint16_t)(master | ((uint16_t)slave << 8));
}

void pic_set_mask(uint16_t mask) {
    io_out8(PIC1_DATA, (uint8_t)(mask & 0xFF));
    io_out8(PIC2_DATA, (uint8_t)((mask >> 8) & 0xFF));
}
