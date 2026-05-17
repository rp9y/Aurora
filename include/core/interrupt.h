#ifndef CORE_INTERRUPT_H
#define CORE_INTERRUPT_H

#include <stdint.h>

void interrupt_init(void);
void interrupt_enable(void);
void interrupt_disable(void);
uint64_t interrupt_ticks(void);

#endif
