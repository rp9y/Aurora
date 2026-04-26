#include <aurora/arch/x86_64/io.h>
#include <aurora/drivers/pit.h>

enum {
    PIT_COMMAND_PORT = 0x43U,
    PIT_CHANNEL0_PORT = 0x40U,
    PIT_BASE_FREQUENCY = 1193182U,
    PIT_COMMAND_MODE3 = 0x36U
};

static volatile uint64_t g_pit_ticks = 0U;

void pit_init(uint32_t frequency_hz) {
    uint32_t hz = frequency_hz;
    if (hz == 0U) {
        hz = 100U;
    }

    uint32_t divisor = PIT_BASE_FREQUENCY / hz;
    if (divisor == 0U) {
        divisor = 1U;
    }
    if (divisor > 0xFFFFU) {
        divisor = 0xFFFFU;
    }

    outb(PIT_COMMAND_PORT, PIT_COMMAND_MODE3);
    outb(PIT_CHANNEL0_PORT, (uint8_t)(divisor & 0xFFU));
    outb(PIT_CHANNEL0_PORT, (uint8_t)((divisor >> 8U) & 0xFFU));
}

void pit_handle_irq(void) {
    ++g_pit_ticks;
}

uint64_t pit_ticks_get(void) {
    return g_pit_ticks;
}
