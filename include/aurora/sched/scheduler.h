#ifndef AURORA_SCHED_SCHEDULER_H
#define AURORA_SCHED_SCHEDULER_H

#include <aurora/core/types.h>

typedef void (*task_entry_t)(void *arg);

void scheduler_init(void);
int32_t scheduler_create(task_entry_t entry, void *arg);
int32_t scheduler_create_named(const char *name, task_entry_t entry, void *arg);
void scheduler_yield(void);
void scheduler_sleep(uint64_t ticks);
void scheduler_tick(void);
void scheduler_request_reschedule(void);
void scheduler_maybe_preempt(void);
uint64_t scheduler_ticks(void);
int32_t scheduler_current_task(void);
const char *scheduler_task_name(int32_t task_id);
bool scheduler_task_active(int32_t task_id);
const char *scheduler_task_state(int32_t task_id);
uint64_t scheduler_task_wakeup_tick(int32_t task_id);
int32_t scheduler_task_capacity(void);
void scheduler_dump_state(void);

#endif
