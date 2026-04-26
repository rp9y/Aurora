#include <aurora/core/libk/init.h>
#include <aurora/libk/ring.h>

static bool g_libk_ready = false;

void core_libk_init(void) {
    uint8_t scratch[16];
    libk_ring_t ring;

    g_libk_ready = libk_ring_init(&ring, scratch, sizeof(scratch));
}

bool core_libk_ready(void) {
    return g_libk_ready;
}
