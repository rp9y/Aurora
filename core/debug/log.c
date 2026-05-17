#include "core/debug/log.h"

#include "core/debug/serial.h"
#include "core/libk/format.h"

static log_level_t g_level = LOG_LEVEL_INFO;

static const char* level_name(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_WARN:
            return "WARN";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        default:
            return "LOG";
    }
}

void log_set_level(log_level_t level) {
    g_level = level;
}

log_level_t log_get_level(void) {
    return g_level;
}

void log_write(log_level_t level, const char* message) {
    if (level < g_level) {
        return;
    }
    serial_write("[");
    serial_write(level_name(level));
    serial_write("] ");
    serial_write(message);
    serial_write("\r\n");
}

void log_vprintf(log_level_t level, const char* fmt, va_list args) {
    if (level < g_level) {
        return;
    }
    char buffer[512];
    kvsnprintf(buffer, sizeof(buffer), fmt, args);
    log_write(level, buffer);
}

void log_printf(log_level_t level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_vprintf(level, fmt, args);
    va_end(args);
}
