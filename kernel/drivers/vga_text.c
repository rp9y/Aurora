#include <aurora/drivers/vga_text.h>

enum {
    VGA_WIDTH = 80U,
    VGA_HEIGHT = 25U
};

static volatile uint16_t *const g_vga_buffer = (volatile uint16_t *)(uintptr_t)0xB8000U;
static size_t g_row = 0U;
static size_t g_col = 0U;
static uint8_t g_color = 0x0FU;

static uint16_t make_cell(char c, uint8_t color) {
    return (uint16_t)((uint16_t)color << 8U) | (uint8_t)c;
}

static void newline(void) {
    g_col = 0U;
    ++g_row;
    if (g_row < VGA_HEIGHT) {
        return;
    }

    for (size_t y = 1U; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0U; x < VGA_WIDTH; ++x) {
            g_vga_buffer[(y - 1U) * VGA_WIDTH + x] = g_vga_buffer[y * VGA_WIDTH + x];
        }
    }

    for (size_t x = 0U; x < VGA_WIDTH; ++x) {
        g_vga_buffer[(VGA_HEIGHT - 1U) * VGA_WIDTH + x] = make_cell(' ', g_color);
    }

    g_row = VGA_HEIGHT - 1U;
}

void vga_text_set_color(uint8_t fg, uint8_t bg) {
    g_color = (uint8_t)((bg << 4U) | (fg & 0x0FU));
}

void vga_text_clear(void) {
    for (size_t y = 0U; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0U; x < VGA_WIDTH; ++x) {
            g_vga_buffer[y * VGA_WIDTH + x] = make_cell(' ', g_color);
        }
    }
    g_row = 0U;
    g_col = 0U;
}

void vga_text_init(void) {
    vga_text_set_color(0x0FU, 0x00U);
    vga_text_clear();
}

void vga_text_write_char(char c) {
    if (c == '\n') {
        newline();
        return;
    }

    if (c == '\r') {
        g_col = 0U;
        return;
    }

    if (c == '\b') {
        if (g_col > 0U) {
            --g_col;
            g_vga_buffer[g_row * VGA_WIDTH + g_col] = make_cell(' ', g_color);
        }
        return;
    }

    g_vga_buffer[g_row * VGA_WIDTH + g_col] = make_cell(c, g_color);
    ++g_col;
    if (g_col >= VGA_WIDTH) {
        newline();
    }
}

void vga_text_write(const char *text) {
    if (text == (const char *)0) {
        return;
    }

    for (size_t i = 0U; text[i] != '\0'; ++i) {
        vga_text_write_char(text[i]);
    }
}
