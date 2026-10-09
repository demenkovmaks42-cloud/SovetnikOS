#ifndef SOVETNIK_MOUSE_H
#define SOVETNIK_MOUSE_H

#include "types.h"

typedef struct {
    int x;
    int y;
    int left;
    int right;
    int middle;
} MouseState;

void mouse_init(void);
void mouse_poll(void);
MouseState mouse_get_state(void);

#endif
