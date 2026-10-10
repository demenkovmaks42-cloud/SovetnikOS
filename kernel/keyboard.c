#include "keyboard.h"
#include "types.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* US QWERTY, нижний регистр */
static const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,  '*', 0,  ' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* Sкан-коды для стрелок и др. после префикса 0xE0 */
#define SC_UP     0x48
#define SC_DOWN   0x50
#define SC_LEFT   0x4B
#define SC_RIGHT  0x4D
#define SC_DELETE 0x53
#define SC_HOME   0x47
#define SC_END    0x4F

static int extended = 0;   /* был ли префикс 0xE0 */

void keyboard_init(void) {
    while (inb(KBD_STATUS) & 0x01) inb(KBD_DATA);
    extended = 0;
}

int keyboard_getkey(void) {
    while (1) {
        if (!(inb(KBD_STATUS) & 0x01)) continue;
        uint8_t sc = inb(KBD_DATA);

        /* префикс extended */
        if (sc == 0xE0) { extended = 1; continue; }

        /* отпускание */
        if (sc & 0x80) {
            extended = 0;
            continue;
        }

        if (extended) {
            extended = 0;
            switch (sc) {
                case SC_UP:     return KEY_UP;
                case SC_DOWN:   return KEY_DOWN;
                case SC_LEFT:   return KEY_LEFT;
                case SC_RIGHT:  return KEY_RIGHT;
                case SC_DELETE: return KEY_DELETE;
                case SC_HOME:   return KEY_HOME;
                case SC_END:    return KEY_END;
            }
            continue;
        }

        char c = keymap[sc & 0x7F];
        if (c) return (int)(uint8_t)c;
    }
}

char keyboard_getchar(void) {
    while (1) {
        int k = keyboard_getkey();
        if (k > 0 && k < 0x100) return (char)k;
    }
}
