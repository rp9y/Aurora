#include "core/scheduler.h"

#include "core/debug/log.h"
#include "core/libk/string.h"

enum {
    MAX_TASKS = 64,
};

static task_info_t g_tasks[MAX_TASKS];
static uint32_t g_task_count;
static uint32_t g_current_slot;
static uint32_t g_next_task_id;

static int find_slot_by_id(uint32_t id) {
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].state != TASK_UNUSED && g_tasks[i].id == id) {
            return (int)i;
        }
    }
    return -1;
}

void scheduler_init(void) {
    kmemset(g_tasks, 0, sizeof(g_tasks));
    g_task_count = 0;
    g_current_slot = 0;
    g_next_task_id = 1;
}

uint32_t scheduler_create_task(const char* name) {
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].state == TASK_UNUSED) {
            task_info_t* task = &g_tasks[i];
            task->id = g_next_task_id++;
            kstrncpy(task->name, name != 0 ? name : "task", sizeof(task->name) - 1u);
            task->name[sizeof(task->name) - 1u] = '\0';
            task->state = TASK_READY;
            task->run_ticks = 0;
            task->sleep_until_tick = 0;
            g_task_count++;
            return task->id;
        }
    }
    return 0;
}

bool scheduler_sleep_task(uint32_t task_id, uint64_t wake_tick) {
    const int slot = find_slot_by_id(task_id);
    if (slot < 0) {
        return false;
    }
    g_tasks[slot].state = TASK_SLEEPING;
    g_tasks[slot].sleep_until_tick = wake_tick;
    return true;
}

void scheduler_tick(uint64_t now_tick) {
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].state == TASK_SLEEPING && g_tasks[i].sleep_until_tick <= now_tick) {
            g_tasks[i].state = TASK_READY;
        }
    }

    for (uint32_t scan = 0; scan < MAX_TASKS; scan++) {
        g_current_slot = (g_current_slot + 1u) % MAX_TASKS;
        if (g_tasks[g_current_slot].state == TASK_READY || g_tasks[g_current_slot].state == TASK_RUNNING) {
            g_tasks[g_current_slot].state = TASK_RUNNING;
            g_tasks[g_current_slot].run_ticks++;
            return;
        }
    }
}

uint32_t scheduler_current_task(void) {
    if (g_tasks[g_current_slot].state == TASK_UNUSED) {
        return 0;
    }
    return g_tasks[g_current_slot].id;
}

uint32_t scheduler_task_count(void) {
    return g_task_count;
}

bool scheduler_get_task(uint32_t task_id, task_info_t* out) {
    const int slot = find_slot_by_id(task_id);
    if (slot < 0 || out == 0) {
        return false;
    }
    *out = g_tasks[slot];
    return true;
}

void scheduler_dump_tasks(void) {
    log_write(LOG_LEVEL_INFO, "scheduler: task list");
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].state == TASK_UNUSED) {
            continue;
        }
        log_printf(LOG_LEVEL_INFO, "  id=%u name=%s state=%u ticks=%u",
                   (unsigned)g_tasks[i].id,
                   g_tasks[i].name,
                   (unsigned)g_tasks[i].state,
                   (unsigned)g_tasks[i].run_ticks);
    }
}
