#ifndef CORE_INIT_STAGE_H
#define CORE_INIT_STAGE_H

#include <stdint.h>

void init_stage_begin(const char* name);
void init_stage_end(const char* name, uint64_t tick_snapshot);

#endif
