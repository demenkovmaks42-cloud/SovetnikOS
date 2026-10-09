#ifndef SOVETNIK_CONSOLE_H
#define SOVETNIK_CONSOLE_H

#include "types.h"

void console_puts(const char *s);
void console_putc(char c);
void console_clear(void);
void console_set_color(uint8_t color);
void console_prompt(void);

#endif
