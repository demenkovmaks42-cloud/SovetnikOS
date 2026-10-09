#include "../include/keyboard.h"
#include "../include/ports.h"

void keyboard_init(void)
{
}

int keyboard_has_data(void)
{
    return (inb(0x64) & 1) != 0;
}

uint8_t keyboard_read_scancode(void)
{
    return inb(0x60);
}

char keyboard_scancode_to_ascii(uint8_t s)
{
    static const char table[] = {
        0, 27,
        '1','2','3','4','5','6','7','8','9','0',
        '-','=',
        '\b','\t',
        'q','w','e','r','t','y','u','i','o','p',
        '[',']','\n',0,
        'a','s','d','f','g','h','j','k','l',
        ';','\'','`',0,'\\',
        'z','x','c','v','b','n','m',
        ',','.','/',0,'*',0,' '
    };

    if (s < sizeof(table))
        return table[s];

    return 0;
}
