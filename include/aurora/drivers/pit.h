#ifndef AURORA_DRIVERS_PIT_H
#define AURORA_DRIVERS_PIT_H

#include <aurora/core/types.h>

void pit_init(uint32_t frequency_hz);
void pit_handle_irq(void);
uint64_t pit_ticks_get(void);

#endif
