#include <aurora/core/debug/init.h>
#include <aurora/debug/trace.h>

void core_debug_init(void) {
    debug_trace_init();
    debug_trace_write("[aurora] debug trace online\n");
}

void core_debug_flush(void) {
    debug_trace_dump_serial();
}
