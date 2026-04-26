#ifndef AURORA_MM_KHEAP_H
#define AURORA_MM_KHEAP_H

#include <aurora/core/types.h>

void kheap_init(void);
void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *ptr, size_t new_size);
void kfree(void *ptr);
uint64_t kheap_bytes_total(void);
uint64_t kheap_bytes_used(void);

#endif
