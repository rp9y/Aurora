#include <aurora/debug/trace.h>
#include <aurora/drivers/serial.h>
#include <aurora/libk/ring.h>

enum {
    TRACE_BUFFER_SIZE = 4096U
};

static uint8_t g_trace_storage[TRACE_BUFFER_SIZE];
static libk_ring_t g_trace_ring;
static bool g_trace_ready = false;

void debug_trace_init(void) {
    g_trace_ready = libk_ring_init(&g_trace_ring, g_trace_storage, TRACE_BUFFER_SIZE);
}

void debug_trace_write_n(const char *text, size_t length) {
    if (!g_trace_ready || text == (const char *)0) {
        return;
    }

    for (size_t i = 0U; i < length; ++i) {
        const uint8_t byte = (uint8_t)text[i];
        if (!libk_ring_push(&g_trace_ring, byte)) {
            uint8_t dropped = 0U;
            (void)libk_ring_pop(&g_trace_ring, &dropped);
            (void)libk_ring_push(&g_trace_ring, byte);
        }
    }
}

void debug_trace_write(const char *text) {
    if (text == (const char *)0) {
        return;
    }

    for (size_t i = 0U; text[i] != '\0'; ++i) {
        debug_trace_write_n(&text[i], 1U);
    }
}

void debug_trace_dump_serial(void) {
    if (!g_trace_ready) {
        return;
    }

    uint8_t value = 0U;
    while (libk_ring_pop(&g_trace_ring, &value)) {
        serial_write_char((char)value);
    }
}

size_t debug_trace_count(void) {
    return libk_ring_count(&g_trace_ring);
}
