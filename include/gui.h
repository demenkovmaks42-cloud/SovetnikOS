#ifndef SOVETNIK_GUI_H
#define SOVETNIK_GUI_H

#include "types.h"

void gui_init(void);
void gui_clear(uint8_t color);
void gui_pixel(int x, int y, uint8_t color);
void gui_rect(int x, int y, int w, int h, uint8_t color);
void gui_frame(int x, int y, int w, int h, uint8_t color);
void gui_cursor(int x, int y);

#endif
