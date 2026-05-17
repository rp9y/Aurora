#ifndef AURORA_ARCH_X86_64_IO_H
#define AURORA_ARCH_X86_64_IO_H

#include <stdint.h>

uint8_t io_in8(uint16_t port);
uint16_t io_in16(uint16_t port);
uint32_t io_in32(uint16_t port);
void io_out8(uint16_t port, uint8_t value);
void io_out16(uint16_t port, uint16_t value);
void io_out32(uint16_t port, uint32_t value);
void io_wait(void);

#endif
