#include "keyboard.h"
#include "types.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

static uint8_t shift_down = 0;
static uint8_t caps_lock = 0;
static uint8_t extended = 0;

static const char keymap[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/',
    0, '*', 0, ' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const char shifted[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?',
    0, '*', 0, ' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void keyboard_init(void)
{
    uint32_t guard = 100000u;
    while ((inb(KBD_STATUS) & 1u) && guard--) (void)inb(KBD_DATA);
    shift_down = 0;
    caps_lock = 0;
    extended = 0;
}

int keyboard_getkey(void)
{
    for (;;) {
        uint8_t sc;
        if (!(inb(KBD_STATUS) & 1u)) {
            __asm__ volatile ("pause");
            continue;
        }
        sc = inb(KBD_DATA);
        if (sc == 0xE0) { extended = 1; continue; }

        if (sc == 0x2A || sc == 0x36) { shift_down = 1; extended = 0; continue; }
        if (sc == 0xAA || sc == 0xB6) { shift_down = 0; extended = 0; continue; }
        if (sc == 0x3A) { caps_lock ^= 1u; extended = 0; continue; }

        if (sc & 0x80u) { extended = 0; continue; }
        if (extended) {
            extended = 0;
            switch (sc) {
                case 0x48: return KEY_UP;
                case 0x50: return KEY_DOWN;
                case 0x4B: return KEY_LEFT;
                case 0x4D: return KEY_RIGHT;
                case 0x53: return KEY_DELETE;
                case 0x47: return KEY_HOME;
                case 0x4F: return KEY_END;
                default: continue;
            }
        }

        if (sc >= 128) continue;
        {
            char c = (shift_down ? shifted[sc] : keymap[sc]);
            if (caps_lock && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            else if (caps_lock && shift_down && c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            if (c) return (int)(uint8_t)c;
        }
    }
}

char keyboard_getchar(void)
{
    for (;;) {
        int key = keyboard_getkey();
        if (key > 0 && key < 0x100) return (char)key;
    }
}
