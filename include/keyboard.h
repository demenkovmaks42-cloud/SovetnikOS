#ifndef KEYBOARD_H
#define KEYBOARD_H

/* Специальные коды, которые возвращает keyboard_getkey() */
#define KEY_NONE    0
#define KEY_UP      0x100
#define KEY_DOWN    0x101
#define KEY_LEFT    0x102
#define KEY_RIGHT   0x103
#define KEY_DELETE  0x104
#define KEY_HOME    0x105
#define KEY_END     0x106

/* Возвращает ASCII или KEY_*; 0 = ничего */
int  keyboard_getkey(void);

/* Совместимость: возвращает только ASCII, блокирующе */
char keyboard_getchar(void);

void keyboard_init(void);

#endif
