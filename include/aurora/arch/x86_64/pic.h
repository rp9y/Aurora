#ifndef AURORA_ARCH_X86_64_PIC_H
#define AURORA_ARCH_X86_64_PIC_H

#include <stdint.h>

enum {
    PIC1_COMMAND = 0x20,
    PIC1_DATA = 0x21,
    PIC2_COMMAND = 0xA0,
    PIC2_DATA = 0xA1,
};

void pic_remap(uint8_t master_offset, uint8_t slave_offset);
void pic_send_eoi(uint8_t irq);
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);
uint16_t pic_get_mask(void);
void pic_set_mask(uint16_t mask);

#endif
