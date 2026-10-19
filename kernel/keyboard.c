#include "keyboard.h"

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64

#define KEY_BUFFER_SIZE 256

static char key_buffer[KEY_BUFFER_SIZE];
static volatile uint32_t key_head = 0;
static volatile uint32_t key_tail = 0;

static volatile int shift_down = 0;
static volatile int ctrl_down = 0;
static volatile int alt_down = 0;
static volatile int caps_on = 0;

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static void buffer_put(char c)
{
    uint32_t next = (key_head + 1u) % KEY_BUFFER_SIZE;

    if (next == key_tail)
        return;

    key_buffer[key_head] = c;
    key_head = next;
}

static int is_letter(char c)
{
    return c >= 'a' && c <= 'z';
}

static char translate_scancode(uint8_t sc)
{
    static const char normal[128] = {
        [0x02] = '1', [0x03] = '2', [0x04] = '3',
        [0x05] = '4', [0x06] = '5', [0x07] = '6',
        [0x08] = '7', [0x09] = '8', [0x0A] = '9',
        [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
        [0x0E] = '\b', [0x0F] = '\t',
        [0x10] = 'q', [0x11] = 'w', [0x12] = 'e',
        [0x13] = 'r', [0x14] = 't', [0x15] = 'y',
        [0x16] = 'u', [0x17] = 'i', [0x18] = 'o',
        [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
        [0x1C] = '\n',
        [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd',
        [0x21] = 'f', [0x22] = 'g', [0x23] = 'h',
        [0x24] = 'j', [0x25] = 'k', [0x26] = 'l',
        [0x27] = ';', [0x28] = '\'',
        [0x29] = '`', [0x2B] = '\\',
        [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c',
        [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n',
        [0x32] = 'm', [0x33] = ',', [0x34] = '.',
        [0x35] = '/', [0x39] = ' '
    };

    static const char shifted[128] = {
        [0x02] = '!', [0x03] = '@', [0x04] = '#',
        [0x05] = '$', [0x06] = '%', [0x07] = '^',
        [0x08] = '&', [0x09] = '*', [0x0A] = '(',
        [0x0B] = ')', [0x0C] = '_', [0x0D] = '+',
        [0x0E] = '\b', [0x0F] = '\t',
        [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E',
        [0x13] = 'R', [0x14] = 'T', [0x15] = 'Y',
        [0x16] = 'U', [0x17] = 'I', [0x18] = 'O',
        [0x19] = 'P', [0x1A] = '{', [0x1B] = '}',
        [0x1C] = '\n',
        [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D',
        [0x21] = 'F', [0x22] = 'G', [0x23] = 'H',
        [0x24] = 'J', [0x25] = 'K', [0x26] = 'L',
        [0x27] = ':', [0x28] = '"',
        [0x29] = '~', [0x2B] = '|',
        [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C',
        [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N',
        [0x32] = 'M', [0x33] = '<', [0x34] = '>',
        [0x35] = '?', [0x39] = ' '
    };

    if (sc >= 128)
        return 0;

    char c = shift_down ? shifted[sc] : normal[sc];

    if (is_letter(normal[sc])) {
        int uppercase = shift_down ^ caps_on;
        c = uppercase
        ? (char)(normal[sc] - 'a' + 'A')
        : normal[sc];
    }

    return c;
}

void keyboard_init(void)
{
    key_head = 0;
    key_tail = 0;

    shift_down = 0;
    ctrl_down = 0;
    alt_down = 0;
    caps_on = 0;

    /*
     * Не очищаем порт 0x60 здесь:
     * его выходной буфер может содержать байт,
     * предназначенный для другого PS/2-устройства.
     */
}

void keyboard_push_byte(uint8_t byte)
{
    /*
     * Игнорируем специальные префиксы расширенных
     * сканкодов в этой базовой версии.
     */
    static int extended = 0;

    if (byte == 0xE0 || byte == 0xE1) {
        extended = 1;
        return;
    }

    if (extended) {
        extended = 0;
        if (byte & 0x80u) return; /* ignore key release */
        switch (byte) {
            case 0x4B: buffer_put(KEYCODE_LEFT);  return;
            case 0x4D: buffer_put(KEYCODE_RIGHT); return;
            case 0x48: buffer_put(KEYCODE_UP);    return;
            case 0x50: buffer_put(KEYCODE_DOWN);  return;
            default: return;
        }
    }

    int released = (byte & 0x80u) != 0;
    uint8_t sc = byte & 0x7Fu;

    switch (sc) {
        case 0x2A: /* Left Shift */
        case 0x36: /* Right Shift */
            shift_down = !released;
            return;

        case 0x1D: /* Ctrl */
            ctrl_down = !released;
            return;

        case 0x38: /* Alt */
            alt_down = !released;
            return;

        case 0x3A: /* Caps Lock */
            if (!released)
                caps_on = !caps_on;
        return;

        default:
            break;
    }

    if (released)
        return;

    char c = translate_scancode(sc);

    if (c != 0)
        buffer_put(c);
}

int keyboard_has_key(void)
{
    return key_head != key_tail;
}

int keyboard_read_char(char *out)
{
    if (out == 0 || key_tail == key_head)
        return 0;

    *out = key_buffer[key_tail];
    key_tail = (key_tail + 1u) % KEY_BUFFER_SIZE;

    return 1;
}

char keyboard_getchar(void)
{
    char c;

    if (!keyboard_read_char(&c))
        return 0;

    return c;
}

int keyboard_shift_pressed(void)
{
    return shift_down;
}

int keyboard_ctrl_pressed(void)
{
    return ctrl_down;
}

int keyboard_alt_pressed(void)
{
    return alt_down;
}

int keyboard_caps_lock(void)
{
    return caps_on;
}
