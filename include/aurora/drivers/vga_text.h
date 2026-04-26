#ifndef AURORA_DRIVERS_VGA_TEXT_H
#define AURORA_DRIVERS_VGA_TEXT_H

#include <aurora/core/types.h>

void vga_text_init(void);
void vga_text_clear(void);
void vga_text_set_color(uint8_t fg, uint8_t bg);
void vga_text_write_char(char c);
void vga_text_write(const char *text);

#endif
