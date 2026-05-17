#ifndef CORE_LIBK_FORMAT_H
#define CORE_LIBK_FORMAT_H

#include <stdarg.h>
#include <stddef.h>

int kvsnprintf(char* buffer, size_t buffer_size, const char* fmt, va_list args);
int ksnprintf(char* buffer, size_t buffer_size, const char* fmt, ...);

#endif
