#include <aurora/arch/x86_64/io.h>
#include <aurora/arch/x86_64/isr.h>
#include <aurora/arch/x86_64/pic.h>
#include <aurora/drivers/keyboard.h>

enum {
    KBD_DATA_PORT = 0x60U,
    KBD_STATUS_PORT = 0x64U,
    KBD_STATUS_OUTPUT_FULL = 1U << 0U,
    KBD_IRQ_VECTOR = 33U,
    KBD_QUEUE_SIZE = 256U
};

static char g_queue[KBD_QUEUE_SIZE];
static uint32_t g_queue_head = 0U;
static uint32_t g_queue_tail = 0U;
static bool g_shift_held = false;

static const char g_keymap[128] = {
    [0x01] = 0x1B,
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',
    [0x0C] = '-',
    [0x0D] = '=',
    [0x0E] = '\b',
    [0x0F] = '\t',
    [0x10] = 'q',
    [0x11] = 'w',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',
    [0x1A] = '[',
    [0x1B] = ']',
    [0x1C] = '\n',
    [0x1E] = 'a',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = ';',
    [0x28] = '\'',
    [0x29] = '`',
    [0x2B] = '\\',
    [0x2C] = 'z',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = 'm',
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',
    [0x39] = ' '
};

static const char g_keymap_shift[128] = {
    [0x01] = 0x1B,
    [0x02] = '!',
    [0x03] = '@',
    [0x04] = '#',
    [0x05] = '$',
    [0x06] = '%',
    [0x07] = '^',
    [0x08] = '&',
    [0x09] = '*',
    [0x0A] = '(',
    [0x0B] = ')',
    [0x0C] = '_',
    [0x0D] = '+',
    [0x0E] = '\b',
    [0x0F] = '\t',
    [0x10] = 'Q',
    [0x11] = 'W',
    [0x12] = 'E',
    [0x13] = 'R',
    [0x14] = 'T',
    [0x15] = 'Y',
    [0x16] = 'U',
    [0x17] = 'I',
    [0x18] = 'O',
    [0x19] = 'P',
    [0x1A] = '{',
    [0x1B] = '}',
    [0x1C] = '\n',
    [0x1E] = 'A',
    [0x1F] = 'S',
    [0x20] = 'D',
    [0x21] = 'F',
    [0x22] = 'G',
    [0x23] = 'H',
    [0x24] = 'J',
    [0x25] = 'K',
    [0x26] = 'L',
    [0x27] = ':',
    [0x28] = '"',
    [0x29] = '~',
    [0x2B] = '|',
    [0x2C] = 'Z',
    [0x2D] = 'X',
    [0x2E] = 'C',
    [0x2F] = 'V',
    [0x30] = 'B',
    [0x31] = 'N',
    [0x32] = 'M',
    [0x33] = '<',
    [0x34] = '>',
    [0x35] = '?',
    [0x39] = ' '
};

static bool queue_is_full(void) {
    const uint32_t next = (g_queue_head + 1U) % KBD_QUEUE_SIZE;
    return next == g_queue_tail;
}

static bool queue_is_empty(void) {
    return g_queue_head == g_queue_tail;
}

static void queue_push(char c) {
    if (queue_is_full()) {
        return;
    }
    g_queue[g_queue_head] = c;
    g_queue_head = (g_queue_head + 1U) % KBD_QUEUE_SIZE;
}

static int32_t queue_pop(void) {
    if (queue_is_empty()) {
        return -1;
    }

    const char c = g_queue[g_queue_tail];
    g_queue_tail = (g_queue_tail + 1U) % KBD_QUEUE_SIZE;
    return (int32_t)(uint8_t)c;
}

static void keyboard_irq_handler(isr_frame_t *frame) {
    (void)frame;

    if ((inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT_FULL) == 0U) {
        return;
    }

    const uint8_t scancode = inb(KBD_DATA_PORT);
    const bool released = (scancode & 0x80U) != 0U;
    const uint8_t code = (uint8_t)(scancode & 0x7FU);

    if (code == 0x2AU || code == 0x36U) {
        g_shift_held = !released;
        return;
    }

    if (released || code >= 128U) {
        return;
    }

    const char c = g_shift_held ? g_keymap_shift[code] : g_keymap[code];
    if (c != '\0') {
        queue_push(c);
    }
}

void keyboard_init(void) {
    g_queue_head = 0U;
    g_queue_tail = 0U;
    g_shift_held = false;

    isr_register_handler(KBD_IRQ_VECTOR, keyboard_irq_handler);
    pic_clear_irq_mask(1U);
}

int32_t keyboard_getchar(void) {
    return queue_pop();
}

bool keyboard_has_data(void) {
    return !queue_is_empty();
}
