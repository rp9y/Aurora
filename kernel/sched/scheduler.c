#include <aurora/arch/x86_64/cpu.h>
#include <aurora/core/printk.h>
#include <aurora/lib/string.h>
#include <aurora/sched/scheduler.h>

enum {
    MAX_TASKS = 16,
    TASK_STACK_SIZE = 16384,
    TASK_NAME_MAX = 32
};

typedef enum task_state {
    TASK_UNUSED = 0,
    TASK_RUNNABLE = 1,
    TASK_RUNNING = 2,
    TASK_SLEEPING = 3,
    TASK_DEAD = 4
} task_state_t;

typedef struct task_context {
    uint64_t rsp;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
} task_context_t;

typedef struct task {
    int32_t id;
    task_state_t state;
    task_entry_t entry;
    void *arg;
    uint64_t wakeup_tick;
    char name[TASK_NAME_MAX];
    task_context_t context;
    uint8_t stack[TASK_STACK_SIZE];
} task_t;

extern void context_switch(task_context_t *from, task_context_t *to);

static task_t g_tasks[MAX_TASKS];
static int32_t g_current_task = 0;
static volatile uint64_t g_ticks = 0U;
static volatile bool g_need_resched = false;

static void task_bootstrap(void);

static void idle_task(void *arg) {
    (void)arg;
    for (;;) {
        cpu_hlt();
        scheduler_yield();
    }
}

static int32_t scheduler_find_slot(void) {
    for (int32_t i = 1; i < MAX_TASKS; ++i) {
        if (g_tasks[i].state == TASK_UNUSED || g_tasks[i].state == TASK_DEAD) {
            return i;
        }
    }
    return -1;
}

static int32_t scheduler_pick_next(void) {
    for (int32_t offset = 1; offset < MAX_TASKS; ++offset) {
        const int32_t index = (g_current_task + offset) % MAX_TASKS;
        if (g_tasks[index].state == TASK_RUNNABLE) {
            return index;
        }
    }

    if (g_tasks[g_current_task].state == TASK_RUNNING || g_tasks[g_current_task].state == TASK_RUNNABLE) {
        return g_current_task;
    }

    return 0;
}

static void task_bootstrap(void) {
    const int32_t index = g_current_task;
    task_t *task = &g_tasks[index];

    task->entry(task->arg);
    task->state = TASK_DEAD;

    for (;;) {
        scheduler_yield();
    }
}

void scheduler_init(void) {
    aurora_memset(g_tasks, 0, sizeof(g_tasks));

    g_tasks[0].id = 0;
    g_tasks[0].state = TASK_RUNNING;
    g_tasks[0].entry = (task_entry_t)0;
    g_tasks[0].arg = (void *)0;

    g_current_task = 0;
    g_ticks = 0U;
    g_need_resched = false;

    (void)scheduler_create_named("idle", idle_task, (void *)0);
}

int32_t scheduler_create(task_entry_t entry, void *arg) {
    return scheduler_create_named("task", entry, arg);
}

int32_t scheduler_create_named(const char *name, task_entry_t entry, void *arg) {
    if (entry == (task_entry_t)0) {
        return -1;
    }

    const int32_t slot = scheduler_find_slot();
    if (slot < 0) {
        return -1;
    }

    task_t *task = &g_tasks[slot];
    aurora_memset(task, 0, sizeof(*task));

    task->id = slot;
    task->state = TASK_RUNNABLE;
    task->entry = entry;
    task->arg = arg;
    task->wakeup_tick = 0U;
    (void)aurora_strlcpy(task->name, name, sizeof(task->name));

    uintptr_t stack_top = (uintptr_t)&task->stack[TASK_STACK_SIZE];
    stack_top &= ~(uintptr_t)0x0FULL;

    task->context.rsp = (uint64_t)stack_top;
    task->context.rip = (uint64_t)(uintptr_t)task_bootstrap;
    return slot;
}

void scheduler_yield(void) {
    cpu_cli();

    const int32_t previous = g_current_task;
    const int32_t next = scheduler_pick_next();

    if (next == previous) {
        cpu_sti();
        return;
    }

    if (g_tasks[previous].state == TASK_RUNNING) {
        g_tasks[previous].state = TASK_RUNNABLE;
    }

    g_tasks[next].state = TASK_RUNNING;
    g_current_task = next;

    cpu_sti();
    context_switch(&g_tasks[previous].context, &g_tasks[next].context);
}

void scheduler_sleep(uint64_t ticks) {
    if (ticks == 0ULL) {
        scheduler_yield();
        return;
    }

    cpu_cli();
    task_t *task = &g_tasks[g_current_task];
    if (task->state == TASK_RUNNING) {
        task->state = TASK_SLEEPING;
        task->wakeup_tick = g_ticks + ticks;
    }
    cpu_sti();

    scheduler_yield();
}

void scheduler_tick(void) {
    ++g_ticks;

    for (int32_t i = 1; i < MAX_TASKS; ++i) {
        task_t *task = &g_tasks[i];
        if (task->state == TASK_SLEEPING && task->wakeup_tick <= g_ticks) {
            task->wakeup_tick = 0U;
            task->state = TASK_RUNNABLE;
            scheduler_request_reschedule();
        }
    }

    if ((g_ticks % 8U) == 0U) {
        scheduler_request_reschedule();
    }
}

void scheduler_request_reschedule(void) {
    g_need_resched = true;
}

void scheduler_maybe_preempt(void) {
    if (g_need_resched) {
        g_need_resched = false;
        scheduler_yield();
    }
}

uint64_t scheduler_ticks(void) {
    return g_ticks;
}

int32_t scheduler_current_task(void) {
    return g_current_task;
}

const char *scheduler_task_name(int32_t task_id) {
    if (task_id < 0 || task_id >= MAX_TASKS) {
        return "";
    }

    if (g_tasks[task_id].state == TASK_UNUSED) {
        return "";
    }

    return g_tasks[task_id].name;
}

static const char *scheduler_state_name(task_state_t state) {
    switch (state) {
        case TASK_UNUSED:
            return "unused";
        case TASK_RUNNABLE:
            return "runnable";
        case TASK_RUNNING:
            return "running";
        case TASK_SLEEPING:
            return "sleeping";
        case TASK_DEAD:
            return "dead";
        default:
            return "unknown";
    }
}

bool scheduler_task_active(int32_t task_id) {
    if (task_id < 0 || task_id >= MAX_TASKS) {
        return false;
    }
    return g_tasks[task_id].state != TASK_UNUSED;
}

const char *scheduler_task_state(int32_t task_id) {
    if (task_id < 0 || task_id >= MAX_TASKS) {
        return "invalid";
    }
    return scheduler_state_name(g_tasks[task_id].state);
}

uint64_t scheduler_task_wakeup_tick(int32_t task_id) {
    if (task_id < 0 || task_id >= MAX_TASKS) {
        return 0ULL;
    }
    return g_tasks[task_id].wakeup_tick;
}

int32_t scheduler_task_capacity(void) {
    return MAX_TASKS;
}

void scheduler_dump_state(void) {
    printk("[aurora] task table tick=%u\n", g_ticks);
    for (int32_t i = 0; i < MAX_TASKS; ++i) {
        const task_t *task = &g_tasks[i];
        if (task->state == TASK_UNUSED) {
            continue;
        }

        printk(
            "[aurora] task id=%u name=%s state=%s wake=%u\n",
            (uint64_t)task->id,
            task->name,
            scheduler_state_name(task->state),
            task->wakeup_tick
        );
    }
}
