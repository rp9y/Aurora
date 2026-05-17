#include "core/libk/string.h"

#include <stdint.h>

size_t kstrlen(const char* text) {
    size_t len = 0;
    if (text == 0) {
        return 0;
    }
    while (text[len] != '\0') {
        len++;
    }
    return len;
}

int kstrcmp(const char* lhs, const char* rhs) {
    size_t i = 0;
    while (lhs[i] != '\0' && rhs[i] != '\0' && lhs[i] == rhs[i]) {
        i++;
    }
    return (int)((unsigned char)lhs[i] - (unsigned char)rhs[i]);
}

int kstrncmp(const char* lhs, const char* rhs, size_t n) {
    for (size_t i = 0; i < n; i++) {
        const unsigned char a = (unsigned char)lhs[i];
        const unsigned char b = (unsigned char)rhs[i];
        if (a != b) {
            return (int)(a - b);
        }
        if (a == '\0') {
            return 0;
        }
    }
    return 0;
}

char* kstrcpy(char* dst, const char* src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

char* kstrncpy(char* dst, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return dst;
}

char* kstrcat(char* dst, const char* src) {
    size_t d = kstrlen(dst);
    size_t s = 0;
    while (src[s] != '\0') {
        dst[d + s] = src[s];
        s++;
    }
    dst[d + s] = '\0';
    return dst;
}

const char* kstrstr(const char* haystack, const char* needle) {
    if (needle == 0 || needle[0] == '\0') {
        return haystack;
    }
    const size_t needle_len = kstrlen(needle);
    const size_t hay_len = kstrlen(haystack);
    if (needle_len > hay_len) {
        return 0;
    }
    for (size_t i = 0; i + needle_len <= hay_len; i++) {
        if (kstrncmp(&haystack[i], needle, needle_len) == 0) {
            return &haystack[i];
        }
    }
    return 0;
}

int katoi(const char* text) {
    if (text == 0) {
        return 0;
    }
    int sign = 1;
    if (*text == '-') {
        sign = -1;
        text++;
    }
    int value = 0;
    while (*text >= '0' && *text <= '9') {
        value = (value * 10) + (*text - '0');
        text++;
    }
    return value * sign;
}

uint64_t katoi_hex(const char* text) {
    if (text == 0) {
        return 0;
    }
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text += 2;
    }
    uint64_t value = 0;
    while (*text != '\0') {
        char c = *text;
        uint8_t d = 0;
        if (c >= '0' && c <= '9') {
            d = (uint8_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            d = (uint8_t)(10 + (c - 'a'));
        } else if (c >= 'A' && c <= 'F') {
            d = (uint8_t)(10 + (c - 'A'));
        } else {
            break;
        }
        value = (value << 4) | d;
        text++;
    }
    return value;
}

void* kmemset(void* dst, int value, size_t size) {
    uint8_t* out = (uint8_t*)dst;
    for (size_t i = 0; i < size; i++) {
        out[i] = (uint8_t)value;
    }
    return dst;
}

void* kmemcpy(void* dst, const void* src, size_t size) {
    uint8_t* out = (uint8_t*)dst;
    const uint8_t* in = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++) {
        out[i] = in[i];
    }
    return dst;
}

int kmemcmp(const void* lhs, const void* rhs, size_t size) {
    const uint8_t* a = (const uint8_t*)lhs;
    const uint8_t* b = (const uint8_t*)rhs;
    for (size_t i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}
