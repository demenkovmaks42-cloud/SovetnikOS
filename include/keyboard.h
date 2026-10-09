#ifndef SOVETNIK_KEYBOARD_H
#define SOVETNIK_KEYBOARD_H

#include "types.h"

void keyboard_init(void);
int keyboard_has_data(void);
uint8_t keyboard_read_scancode(void);
char keyboard_scancode_to_ascii(uint8_t scancode);

#endif
