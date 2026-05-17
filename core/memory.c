#include "core/memory.h"

#include "core/debug/log.h"
#include "core/libk/string.h"

enum {
    KERNEL_HEAP_SIZE = 1024 * 1024,
    KERNEL_HEAP_ALIGN = 16,
};

static uint8_t g_heap[KERNEL_HEAP_SIZE];
static size_t g_heap_offset;

static size_t align_up(size_t value, size_t align) {
    return (value + align - 1u) & ~(align - 1u);
}

void memory_init(void) {
    g_heap_offset = 0;
    kmemset(g_heap, 0, sizeof(g_heap));
    log_printf(LOG_LEVEL_INFO, "memory: heap initialized (%u KiB)", (unsigned)(KERNEL_HEAP_SIZE / 1024));
}

void* memory_alloc(size_t size) {
    if (size == 0) {
        return 0;
    }
    const size_t aligned_size = align_up(size, KERNEL_HEAP_ALIGN);
    const size_t next_offset = align_up(g_heap_offset, KERNEL_HEAP_ALIGN);
    if (next_offset + aligned_size > KERNEL_HEAP_SIZE) {
        log_printf(LOG_LEVEL_WARN, "memory: alloc failed (%u bytes)", (unsigned)size);
        return 0;
    }
    void* ptr = &g_heap[next_offset];
    g_heap_offset = next_offset + aligned_size;
    return ptr;
}

void memory_free(void* ptr) {
    (void)ptr;
}

memory_stats_t memory_stats(void) {
    memory_stats_t stats;
    stats.total_bytes = KERNEL_HEAP_SIZE;
    stats.used_bytes = g_heap_offset;
    stats.free_bytes = KERNEL_HEAP_SIZE - g_heap_offset;
    return stats;
}
