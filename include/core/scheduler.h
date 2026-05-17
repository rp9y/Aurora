#ifndef CORE_SCHEDULER_H
#define CORE_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TASK_UNUSED = 0,
    TASK_READY = 1,
    TASK_RUNNING = 2,
    TASK_SLEEPING = 3,
} task_state_t;

typedef struct {
    uint32_t id;
    char name[32];
    task_state_t state;
    uint64_t run_ticks;
    uint64_t sleep_until_tick;
} task_info_t;

void scheduler_init(void);
uint32_t scheduler_create_task(const char* name);
bool scheduler_sleep_task(uint32_t task_id, uint64_t wake_tick);
void scheduler_tick(uint64_t now_tick);
uint32_t scheduler_current_task(void);
uint32_t scheduler_task_count(void);
bool scheduler_get_task(uint32_t task_id, task_info_t* out);
void scheduler_dump_tasks(void);

#endif
