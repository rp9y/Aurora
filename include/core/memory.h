#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
} memory_stats_t;

void memory_init(void);
void* memory_alloc(size_t size);
void memory_free(void* ptr);
memory_stats_t memory_stats(void);

#endif
