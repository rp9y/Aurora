#include <aurora/arch/x86_64/idt.h>
#include <aurora/arch/x86_64/pic.h>
#include <aurora/core/interrupt/init.h>
#include <aurora/drivers/keyboard.h>
#include <aurora/drivers/pit.h>

void core_interrupt_init(uint32_t pit_frequency_hz, bool enable_keyboard) {
    idt_init();
    pic_init();
    pit_init(pit_frequency_hz);

    if (enable_keyboard) {
        keyboard_init();
    }
}
