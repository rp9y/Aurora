#ifndef AURORA_DRIVERS_SERIAL_H
#define AURORA_DRIVERS_SERIAL_H

#include <aurora/core/types.h>

bool serial_init(void);
void serial_write_char(char c);
void serial_write(const char *text);
void serial_write_n(const char *text, size_t length);
void serial_write_hex_u64(uint64_t value);
void serial_write_dec_u64(uint64_t value);

#endif
