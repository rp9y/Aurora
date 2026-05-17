#include "core/init_stage.h"

#include "core/debug/log.h"
#include "core/interrupt.h"

void init_stage_begin(const char* name) {
    log_printf(LOG_LEVEL_INFO, "init: begin %s", name);
}

void init_stage_end(const char* name, uint64_t tick_snapshot) {
    const uint64_t now = interrupt_ticks();
    log_printf(LOG_LEVEL_INFO, "init: end %s (+%u ticks)", name, (unsigned)(now - tick_snapshot));
}
