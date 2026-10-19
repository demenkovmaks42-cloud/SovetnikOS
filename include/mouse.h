#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"

void mouse_init(void);
void mouse_poll(void);

int mouse_x(void);
int mouse_y(void);

uint8_t mouse_buttons(void);
int mouse_present(void);

void mouse_set_bounds(uint32_t width, uint32_t height);

int mouse_left_pressed(void);
int mouse_right_pressed(void);
int mouse_middle_pressed(void);

#endif
