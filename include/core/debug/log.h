#ifndef CORE_DEBUG_LOG_H
#define CORE_DEBUG_LOG_H

#include <stdarg.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
} log_level_t;

void log_set_level(log_level_t level);
log_level_t log_get_level(void);
void log_write(log_level_t level, const char* message);
void log_printf(log_level_t level, const char* fmt, ...);
void log_vprintf(log_level_t level, const char* fmt, va_list args);

#endif
