#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_NONE    0
#define KEY_UP      0x100
#define KEY_DOWN    0x101
#define KEY_LEFT    0x102
#define KEY_RIGHT   0x103
#define KEY_DELETE  0x104
#define KEY_HOME    0x105
#define KEY_END     0x106

int keyboard_getkey(void);
char keyboard_getchar(void);
void keyboard_init(void);

#endif
