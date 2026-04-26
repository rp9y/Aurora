#ifndef AURORA_CORE_INTERRUPT_INIT_H
#define AURORA_CORE_INTERRUPT_INIT_H

#include <aurora/core/types.h>

void core_interrupt_init(uint32_t pit_frequency_hz, bool enable_keyboard);

#endif
