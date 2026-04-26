#include <aurora/lib/string.h>

void *aurora_memset(void *destination, int value, size_t size) {
    uint8_t *bytes = (uint8_t *)destination;
    const uint8_t fill = (uint8_t)value;

    for (size_t i = 0U; i < size; ++i) {
        bytes[i] = fill;
    }

    return destination;
}

void *aurora_memcpy(void *destination, const void *source, size_t size) {
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;

    for (size_t i = 0U; i < size; ++i) {
        dst[i] = src[i];
    }

    return destination;
}

size_t aurora_strlen(const char *text) {
    size_t length = 0U;

    if (text == NULL) {
        return 0U;
    }

    while (text[length] != '\0') {
        ++length;
    }

    return length;
}

size_t aurora_strnlen(const char *text, size_t max_length) {
    if (text == NULL) {
        return 0U;
    }

    size_t length = 0U;
    while (length < max_length && text[length] != '\0') {
        ++length;
    }
    return length;
}

size_t aurora_strlcpy(char *destination, const char *source, size_t destination_size) {
    if (destination == NULL || destination_size == 0U) {
        return aurora_strlen(source);
    }

    if (source == NULL) {
        destination[0] = '\0';
        return 0U;
    }

    const size_t src_length = aurora_strlen(source);
    const size_t copy_length = (src_length >= destination_size) ? (destination_size - 1U) : src_length;

    aurora_memcpy(destination, source, copy_length);
    destination[copy_length] = '\0';
    return src_length;
}
