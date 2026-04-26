#include <aurora/arch/x86_64/io.h>
#include <aurora/drivers/serial.h>
#include <aurora/drivers/vga_text.h>

enum {
    COM1_BASE = 0x03F8U
};

static bool g_serial_ready = false;

static bool serial_can_transmit(void) {
    const uint8_t status = inb((uint16_t)(COM1_BASE + 5U));
    return (status & 0x20U) != 0U;
}

static bool serial_has_valid_line_status(void) {
    const uint8_t status = inb((uint16_t)(COM1_BASE + 5U));
    return (status & 0x60U) == 0x60U;
}

bool serial_init(void) {
    outb((uint16_t)(COM1_BASE + 1U), 0x00U);
    outb((uint16_t)(COM1_BASE + 3U), 0x80U);
    outb((uint16_t)(COM1_BASE + 0U), 0x03U);
    outb((uint16_t)(COM1_BASE + 1U), 0x00U);
    outb((uint16_t)(COM1_BASE + 3U), 0x03U);
    outb((uint16_t)(COM1_BASE + 2U), 0xC7U);
    outb((uint16_t)(COM1_BASE + 4U), 0x0BU);

    g_serial_ready = serial_has_valid_line_status();
    return g_serial_ready;
}

void serial_write_char(char c) {
    vga_text_write_char(c);

    if (!g_serial_ready) {
        return;
    }

    if (c == '\n') {
        while (!serial_can_transmit()) {
        }
        outb((uint16_t)(COM1_BASE + 0U), (uint8_t)'\r');
    }

    while (!serial_can_transmit()) {
    }
    outb((uint16_t)(COM1_BASE + 0U), (uint8_t)c);
}

void serial_write(const char *text) {
    if (text == NULL) {
        return;
    }

    for (size_t i = 0U; text[i] != '\0'; ++i) {
        serial_write_char(text[i]);
    }
}

void serial_write_n(const char *text, size_t length) {
    if (text == NULL) {
        return;
    }

    for (size_t i = 0U; i < length; ++i) {
        serial_write_char(text[i]);
    }
}

void serial_write_hex_u64(uint64_t value) {
    static const char hex[] = "0123456789ABCDEF";
    char buffer[18];

    buffer[0] = '0';
    buffer[1] = 'x';

    for (uint32_t i = 0U; i < 16U; ++i) {
        const uint32_t shift = (15U - i) * 4U;
        buffer[i + 2U] = hex[(value >> shift) & 0x0FU];
    }

    serial_write_n(buffer, 18U);
}

void serial_write_dec_u64(uint64_t value) {
    char buffer[21];
    size_t length = 0U;

    if (value == 0U) {
        serial_write_char('0');
        return;
    }

    while (value != 0U) {
        const uint64_t digit = value % 10U;
        buffer[length++] = (char)('0' + digit);
        value /= 10U;
    }

    while (length != 0U) {
        --length;
        serial_write_char(buffer[length]);
    }
}
