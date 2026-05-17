#include "core/libk/format.h"

#include <stdbool.h>
#include <stdint.h>

static void append_char(char* buffer, size_t buffer_size, size_t* index, char c) {
    if (*index + 1 < buffer_size) {
        buffer[*index] = c;
    }
    (*index)++;
}

static void append_text(char* buffer, size_t buffer_size, size_t* index, const char* text) {
    if (text == 0) {
        text = "(null)";
    }
    for (size_t i = 0; text[i] != '\0'; i++) {
        append_char(buffer, buffer_size, index, text[i]);
    }
}

static void append_u64_base(char* buffer, size_t buffer_size, size_t* index, uint64_t value, uint32_t base, bool uppercase) {
    static const char* k_digits_lower = "0123456789abcdef";
    static const char* k_digits_upper = "0123456789ABCDEF";
    const char* digits = uppercase ? k_digits_upper : k_digits_lower;

    char tmp[32];
    size_t len = 0;
    if (value == 0) {
        tmp[len++] = '0';
    } else {
        while (value > 0) {
            tmp[len++] = digits[value % base];
            value /= base;
        }
    }
    while (len > 0) {
        append_char(buffer, buffer_size, index, tmp[--len]);
    }
}

int kvsnprintf(char* buffer, size_t buffer_size, const char* fmt, va_list args) {
    size_t index = 0;
    for (size_t i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            append_char(buffer, buffer_size, &index, fmt[i]);
            continue;
        }
        i++;
        if (fmt[i] == '\0') {
            break;
        }
        switch (fmt[i]) {
            case '%':
                append_char(buffer, buffer_size, &index, '%');
                break;
            case 'c': {
                const int value = va_arg(args, int);
                append_char(buffer, buffer_size, &index, (char)value);
                break;
            }
            case 's': {
                const char* value = va_arg(args, const char*);
                append_text(buffer, buffer_size, &index, value);
                break;
            }
            case 'd':
            case 'i': {
                const int64_t value = va_arg(args, int64_t);
                if (value < 0) {
                    append_char(buffer, buffer_size, &index, '-');
                    append_u64_base(buffer, buffer_size, &index, (uint64_t)(-value), 10, false);
                } else {
                    append_u64_base(buffer, buffer_size, &index, (uint64_t)value, 10, false);
                }
                break;
            }
            case 'u': {
                const uint64_t value = va_arg(args, uint64_t);
                append_u64_base(buffer, buffer_size, &index, value, 10, false);
                break;
            }
            case 'x':
            case 'X': {
                const uint64_t value = va_arg(args, uint64_t);
                append_u64_base(buffer, buffer_size, &index, value, 16, fmt[i] == 'X');
                break;
            }
            case 'p': {
                const uintptr_t value = va_arg(args, uintptr_t);
                append_text(buffer, buffer_size, &index, "0x");
                append_u64_base(buffer, buffer_size, &index, (uint64_t)value, 16, false);
                break;
            }
            default:
                append_char(buffer, buffer_size, &index, '%');
                append_char(buffer, buffer_size, &index, fmt[i]);
                break;
        }
    }

    if (buffer_size > 0) {
        const size_t end = (index < buffer_size - 1) ? index : (buffer_size - 1);
        buffer[end] = '\0';
    }
    return (int)index;
}

int ksnprintf(char* buffer, size_t buffer_size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int written = kvsnprintf(buffer, buffer_size, fmt, args);
    va_end(args);
    return written;
}
