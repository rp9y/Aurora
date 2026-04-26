#ifndef AURORA_CORE_SCHEDULER_INIT_H
#define AURORA_CORE_SCHEDULER_INIT_H

#include <aurora/core/types.h>
#include <aurora/sched/scheduler.h>

void core_scheduler_init(void);
int32_t core_scheduler_spawn(const char *name, task_entry_t entry, void *arg);

#endif
