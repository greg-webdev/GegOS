/*
 * keyboard.c - PS/2 Keyboard Driver for GegOS
 * Polling-based keyboard input with scancode translation
 */

#include "keyboard.h"
#include "io.h"

/* PS/2 Keyboard Ports */
#define KB_DATA_PORT    0x60
#define KB_STATUS_PORT  0x64
#define KB_CMD_PORT     0x64

/* Status register bits */
#define KB_STATUS_OUTPUT  0x01
#define KB_STATUS_INPUT   0x02

/* Keyboard state */
static uint8_t modifiers = 0;
static uint8_t key_states[128] = {0};

/* US QWERTY scancode to ASCII (normal) */
static const char scancode_to_ascii[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0
};

/* US QWERTY scancode to ASCII (shifted) */
static const char scancode_to_ascii_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0
};

/* Scancode constants */
#define SC_LSHIFT     0x2A
#define SC_RSHIFT     0x36
#define SC_CTRL       0x1D
#define SC_ALT        0x38
#define SC_CAPSLOCK   0x3A

/* Initialize keyboard */
void keyboard_init(void) {
    /* Clear keyboard buffer */
    while (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT) {
        inb(KB_DATA_PORT);
    }
    
    /* Reset modifier state */
    modifiers = 0;
    
    /* Clear key states */
    for (int i = 0; i < 128; i++) {
        key_states[i] = 0;
    }
}

/* Check if key available (not mouse data) */
int keyboard_haskey(void) {
    uint8_t status = inb(KB_STATUS_PORT);
    /* Bit 0 = data available, Bit 5 = from mouse (must NOT be set) */
    return (status & KB_STATUS_OUTPUT) && !(status & 0x20);
}

/* Get modifier state */
uint8_t keyboard_get_modifiers(void) {
    return modifiers;
}

/* Check if key is held */
int keyboard_key_held(uint8_t scancode) {
    if (scancode < 128) {
        return key_states[scancode];
    }
    return 0;
}

/* Update keyboard state */
void keyboard_update(void) {
    uint8_t status = inb(KB_STATUS_PORT);
    /* Only process if data available AND not from mouse */
    if (!(status & KB_STATUS_OUTPUT) || (status & 0x20)) return;
    
    uint8_t scancode = inb(KB_DATA_PORT);
    int released = (scancode & 0x80) != 0;
    uint8_t key = scancode & 0x7F;
    
    /* Update key state */
    key_states[key] = !released;
    
    /* Update modifiers */
    switch (key) {
        case SC_LSHIFT:
        case SC_RSHIFT:
            if (released) modifiers &= ~MOD_SHIFT;
            else modifiers |= MOD_SHIFT;
            break;
        case SC_CTRL:
            if (released) modifiers &= ~MOD_CTRL;
            else modifiers |= MOD_CTRL;
            break;
        case SC_ALT:
            if (released) modifiers &= ~MOD_ALT;
            else modifiers |= MOD_ALT;
            break;
        case SC_CAPSLOCK:
            if (!released) modifiers ^= MOD_CAPSLOCK;
            break;
    }
}

/* Get character (polling) */
char keyboard_getchar(void) {
    uint8_t status = inb(KB_STATUS_PORT);
    /* Only process if data available AND not from mouse */
    if (!(status & KB_STATUS_OUTPUT) || (status & 0x20)) return 0;
    
    uint8_t scancode = inb(KB_DATA_PORT);
    
    /* Ignore key release */
    if (scancode & 0x80) {
        uint8_t key = scancode & 0x7F;
        key_states[key] = 0;
        
        /* Update modifiers on release */
        switch (key) {
            case SC_LSHIFT:
            case SC_RSHIFT:
                modifiers &= ~MOD_SHIFT;
                break;
            case SC_CTRL:
                modifiers &= ~MOD_CTRL;
                break;
            case SC_ALT:
                modifiers &= ~MOD_ALT;
                break;
        }
        return 0;
    }
    
    /* Update key state */
    key_states[scancode] = 1;
    
    /* Handle modifiers */
    switch (scancode) {
        case SC_LSHIFT:
        case SC_RSHIFT:
            modifiers |= MOD_SHIFT;
            return 0;
        case SC_CTRL:
            modifiers |= MOD_CTRL;
            return 0;
        case SC_ALT:
            modifiers |= MOD_ALT;
            return 0;
        case SC_CAPSLOCK:
            modifiers ^= MOD_CAPSLOCK;
            return 0;
    }
    
    /* Handle arrow keys (extended scancodes) */
    if (scancode == 0x48) return KEY_UP;
    if (scancode == 0x50) return KEY_DOWN;
    if (scancode == 0x4B) return KEY_LEFT;
    if (scancode == 0x4D) return KEY_RIGHT;
    
    /* Handle function keys */
    if (scancode >= 0x3B && scancode <= 0x44) {
        return KEY_F1 + (scancode - 0x3B);
    }
    if (scancode == 0x57) return KEY_F11;
    if (scancode == 0x58) return KEY_F12;
    
    /* Translate to ASCII */
    char c;
    int shift = (modifiers & MOD_SHIFT) != 0;
    int caps = (modifiers & MOD_CAPSLOCK) != 0;
    
    if (shift) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
    }
    
    /* Handle caps lock for letters */
    if (c >= 'a' && c <= 'z' && caps) {
        c -= 32;  /* Convert to uppercase */
    } else if (c >= 'A' && c <= 'Z' && caps && !shift) {
        c += 32;  /* Convert to lowercase when caps+shift */
    }
    
    return c;
}
