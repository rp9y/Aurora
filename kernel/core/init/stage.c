#include <aurora/core/init/stage.h>
#include <aurora/core/printk.h>

void core_init_stage_early(void) {
    printk("[aurora] init stage: early\n");
}

void core_init_stage_post_memory(void) {
    printk("[aurora] init stage: post-memory\n");
}

void core_init_stage_post_scheduler(void) {
    printk("[aurora] init stage: post-scheduler\n");
}
