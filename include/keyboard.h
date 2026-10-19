#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

void keyboard_init(void);

/* Special keys returned by keyboard_read_char/keyboard_getchar. */
#define KEYCODE_LEFT  ((char)1)
#define KEYCODE_RIGHT ((char)2)
#define KEYCODE_UP    ((char)3)
#define KEYCODE_DOWN  ((char)4)

/* Проверка наличия символа в буфере */
int keyboard_has_key(void);

/* Получение следующего символа; 0, если буфер пуст */
char keyboard_getchar(void);

/* Чтение символа без ожидания */
int keyboard_read_char(char *out);

/* Добавление байта из общего PS/2-контроллера */
void keyboard_push_byte(uint8_t byte);

/* Состояние клавиш */
int keyboard_shift_pressed(void);
int keyboard_ctrl_pressed(void);
int keyboard_alt_pressed(void);
int keyboard_caps_lock(void);

#endif
