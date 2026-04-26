#include <aurora/core/printk.h>
#include <aurora/drivers/serial.h>

static void printk_write_unsigned(uint64_t value, uint32_t base, bool uppercase) {
    char buffer[65];
    size_t length = 0U;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (value == 0ULL) {
        serial_write_char('0');
        return;
    }

    while (value != 0ULL) {
        const uint64_t remainder = value % (uint64_t)base;
        buffer[length++] = digits[remainder];
        value /= (uint64_t)base;
    }

    while (length > 0U) {
        --length;
        serial_write_char(buffer[length]);
    }
}

static void printk_write_signed(int64_t value) {
    if (value < 0LL) {
        serial_write_char('-');
        printk_write_unsigned((uint64_t)(-value), 10U, false);
        return;
    }

    printk_write_unsigned((uint64_t)value, 10U, false);
}

void vprintk(const char *format, va_list args) {
    if (format == (const char *)0) {
        return;
    }

    for (size_t i = 0U; format[i] != '\0'; ++i) {
        if (format[i] != '%') {
            serial_write_char(format[i]);
            continue;
        }

        ++i;
        const char specifier = format[i];
        if (specifier == '\0') {
            break;
        }

        switch (specifier) {
            case '%':
                serial_write_char('%');
                break;
            case 'c': {
                const int32_t ch = va_arg(args, int32_t);
                serial_write_char((char)ch);
                break;
            }
            case 's': {
                const char *text = va_arg(args, const char *);
                if (text == (const char *)0) {
                    serial_write("(null)");
                } else {
                    serial_write(text);
                }
                break;
            }
            case 'd':
            case 'i': {
                const int64_t value = va_arg(args, int64_t);
                printk_write_signed(value);
                break;
            }
            case 'u': {
                const uint64_t value = va_arg(args, uint64_t);
                printk_write_unsigned(value, 10U, false);
                break;
            }
            case 'x': {
                const uint64_t value = va_arg(args, uint64_t);
                printk_write_unsigned(value, 16U, false);
                break;
            }
            case 'X': {
                const uint64_t value = va_arg(args, uint64_t);
                printk_write_unsigned(value, 16U, true);
                break;
            }
            case 'p': {
                const uintptr_t value = va_arg(args, uintptr_t);
                serial_write("0x");
                printk_write_unsigned((uint64_t)value, 16U, false);
                break;
            }
            default:
                serial_write_char('%');
                serial_write_char(specifier);
                break;
        }
    }
}

void printk(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintk(format, args);
    va_end(args);
}
