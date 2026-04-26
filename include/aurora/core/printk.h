#ifndef AURORA_CORE_PRINTK_H
#define AURORA_CORE_PRINTK_H

#include <aurora/core/types.h>
#include <stdarg.h>

void vprintk(const char *format, va_list args);
void printk(const char *format, ...);

#endif
