#include <aurora/core/scheduler/init.h>

void core_scheduler_init(void) {
    scheduler_init();
}

int32_t core_scheduler_spawn(const char *name, task_entry_t entry, void *arg) {
    return scheduler_create_named(name, entry, arg);
}
