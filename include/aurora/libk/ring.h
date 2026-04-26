#ifndef AURORA_LIBK_RING_H
#define AURORA_LIBK_RING_H

#include <aurora/core/types.h>

typedef struct libk_ring {
    uint8_t *data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} libk_ring_t;

bool libk_ring_init(libk_ring_t *ring, uint8_t *storage, size_t capacity);
size_t libk_ring_count(const libk_ring_t *ring);
size_t libk_ring_space(const libk_ring_t *ring);
bool libk_ring_push(libk_ring_t *ring, uint8_t value);
bool libk_ring_pop(libk_ring_t *ring, uint8_t *value_out);
void libk_ring_clear(libk_ring_t *ring);

#endif
