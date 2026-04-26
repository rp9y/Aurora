#ifndef AURORA_ARCH_X86_64_PIC_H
#define AURORA_ARCH_X86_64_PIC_H

#include <aurora/core/types.h>

void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_set_irq_mask(uint8_t irq);
void pic_clear_irq_mask(uint8_t irq);

#endif
