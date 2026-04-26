#ifndef AURORA_DRIVERS_KEYBOARD_H
#define AURORA_DRIVERS_KEYBOARD_H

#include <aurora/core/types.h>

void keyboard_init(void);
int32_t keyboard_getchar(void);
bool keyboard_has_data(void);

#endif
