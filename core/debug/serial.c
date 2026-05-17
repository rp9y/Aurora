#include "core/debug/serial.h"

#include "aurora/arch/x86_64/io.h"

enum {
    COM1 = 0x3F8,
    COM_DATA = 0,
    COM_IER = 1,
    COM_FCR = 2,
    COM_LCR = 3,
    COM_MCR = 4,
    COM_LSR = 5,
};

static int serial_ready(void) {
    return (io_in8(COM1 + COM_LSR) & 0x20u) != 0;
}

void serial_init(void) {
    io_out8(COM1 + COM_IER, 0x00);
    io_out8(COM1 + COM_LCR, 0x80);
    io_out8(COM1 + COM_DATA, 0x03);
    io_out8(COM1 + COM_IER, 0x00);
    io_out8(COM1 + COM_LCR, 0x03);
    io_out8(COM1 + COM_FCR, 0xC7);
    io_out8(COM1 + COM_MCR, 0x0B);
}

void serial_write_char(char c) {
    while (!serial_ready()) {
    }
    io_out8(COM1 + COM_DATA, (uint8_t)c);
}

void serial_write(const char* text) {
    if (text == 0) {
        return;
    }
    for (size_t i = 0; text[i] != '\0'; i++) {
        serial_write_char(text[i]);
    }
}

void serial_write_n(const char* text, size_t len) {
    for (size_t i = 0; i < len; i++) {
        serial_write_char(text[i]);
    }
}

void serial_write_hex_u64(uint64_t value) {
    static const char* digits = "0123456789abcdef";
    serial_write("0x");
    for (int i = 60; i >= 0; i -= 4) {
        serial_write_char(digits[(value >> i) & 0xFu]);
    }
}
