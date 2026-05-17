#ifndef CORE_DEBUG_SERIAL_H
#define CORE_DEBUG_SERIAL_H

#include <stddef.h>
#include <stdint.h>

void serial_init(void);
void serial_write_char(char c);
void serial_write(const char* text);
void serial_write_n(const char* text, size_t len);
void serial_write_hex_u64(uint64_t value);

#endif
