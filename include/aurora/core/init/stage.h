#ifndef AURORA_CORE_INIT_STAGE_H
#define AURORA_CORE_INIT_STAGE_H

#include <aurora/core/types.h>

void core_init_stage_early(void);
void core_init_stage_post_memory(void);
void core_init_stage_post_scheduler(void);

#endif
