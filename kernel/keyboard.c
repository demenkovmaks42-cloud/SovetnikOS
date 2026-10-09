#include "keyboard.h"
#include "types.h"

#define KBD_DATA 0x60
#define KBD_STATUS 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

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

void keyboard_init(void) {
    // ждём, пока буфер ввода опустеет
    while (inb(KBD_STATUS) & 0x01) inb(KBD_DATA);
}

char keyboard_getchar(void) {
    while (1) {
        if (!(inb(KBD_STATUS) & 0x01)) continue;
        uint8_t sc = inb(KBD_DATA);
        if (sc & 0x80) continue;       // отпускание
        char c = keymap[sc & 0x7F];
        if (c) return c;
    }
}
