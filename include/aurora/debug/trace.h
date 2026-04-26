#ifndef AURORA_DEBUG_TRACE_H
#define AURORA_DEBUG_TRACE_H

#include <aurora/core/types.h>

void debug_trace_init(void);
void debug_trace_write(const char *text);
void debug_trace_write_n(const char *text, size_t length);
void debug_trace_dump_serial(void);
size_t debug_trace_count(void);

#endif
