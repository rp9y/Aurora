#include <aurora/libk/ring.h>

bool libk_ring_init(libk_ring_t *ring, uint8_t *storage, size_t capacity) {
    if (ring == (libk_ring_t *)0 || storage == (uint8_t *)0 || capacity == 0U) {
        return false;
    }

    ring->data = storage;
    ring->capacity = capacity;
    ring->head = 0U;
    ring->tail = 0U;
    ring->count = 0U;
    return true;
}

size_t libk_ring_count(const libk_ring_t *ring) {
    if (ring == (const libk_ring_t *)0) {
        return 0U;
    }
    return ring->count;
}

size_t libk_ring_space(const libk_ring_t *ring) {
    if (ring == (const libk_ring_t *)0) {
        return 0U;
    }
    return ring->capacity - ring->count;
}

bool libk_ring_push(libk_ring_t *ring, uint8_t value) {
    if (ring == (libk_ring_t *)0 || ring->count == ring->capacity) {
        return false;
    }

    ring->data[ring->head] = value;
    ring->head = (ring->head + 1U) % ring->capacity;
    ++ring->count;
    return true;
}

bool libk_ring_pop(libk_ring_t *ring, uint8_t *value_out) {
    if (ring == (libk_ring_t *)0 || value_out == (uint8_t *)0 || ring->count == 0U) {
        return false;
    }

    *value_out = ring->data[ring->tail];
    ring->tail = (ring->tail + 1U) % ring->capacity;
    --ring->count;
    return true;
}

void libk_ring_clear(libk_ring_t *ring) {
    if (ring == (libk_ring_t *)0) {
        return;
    }
    ring->head = 0U;
    ring->tail = 0U;
    ring->count = 0U;
}
