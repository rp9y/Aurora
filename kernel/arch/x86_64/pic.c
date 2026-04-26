#include <aurora/arch/x86_64/io.h>
#include <aurora/arch/x86_64/pic.h>

enum {
    PIC1_COMMAND = 0x20U,
    PIC1_DATA = 0x21U,
    PIC2_COMMAND = 0xA0U,
    PIC2_DATA = 0xA1U,
    PIC_EOI = 0x20U,

    ICW1_ICW4 = 0x01U,
    ICW1_INIT = 0x10U,
    ICW4_8086 = 0x01U,

    PIC1_OFFSET = 0x20U,
    PIC2_OFFSET = 0x28U
};

static void pic_remap(void) {
    const uint8_t mask1 = inb(PIC1_DATA);
    const uint8_t mask2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, PIC1_OFFSET);
    io_wait();
    outb(PIC2_DATA, PIC2_OFFSET);
    io_wait();

    outb(PIC1_DATA, 0x04U);
    io_wait();
    outb(PIC2_DATA, 0x02U);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8U) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_set_irq_mask(uint8_t irq) {
    uint16_t port = PIC1_DATA;
    if (irq >= 8U) {
        port = PIC2_DATA;
        irq = (uint8_t)(irq - 8U);
    }

    const uint8_t value = (uint8_t)(inb(port) | (1U << irq));
    outb(port, value);
}

void pic_clear_irq_mask(uint8_t irq) {
    uint16_t port = PIC1_DATA;
    if (irq >= 8U) {
        port = PIC2_DATA;
        irq = (uint8_t)(irq - 8U);
    }

    const uint8_t value = (uint8_t)(inb(port) & ~(1U << irq));
    outb(port, value);
}

void pic_init(void) {
    pic_remap();

    outb(PIC1_DATA, 0xFFU);
    outb(PIC2_DATA, 0xFFU);

    pic_clear_irq_mask(0U);
}
