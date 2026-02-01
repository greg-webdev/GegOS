/*
 * keyboard.h - PS/2 Keyboard Driver for GegOS
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* Key state flags */
#define KEY_PRESSED   1
#define KEY_RELEASED  0

/* Modifier flags */
#define MOD_SHIFT     (1 << 0)
#define MOD_CTRL      (1 << 1)
#define MOD_ALT       (1 << 2)
#define MOD_CAPSLOCK  (1 << 3)
#define MOD_SUPER     (1 << 4)  /* Meta/Windows/Super key */

/* Special key codes */
#define KEY_ESCAPE    27
#define KEY_BACKSPACE 8
#define KEY_TAB       9
#define KEY_ENTER     '\n'
#define KEY_UP        128
#define KEY_DOWN      129
#define KEY_LEFT      130
#define KEY_RIGHT     131
#define KEY_F1        132
#define KEY_F2        133
#define KEY_F3        134
#define KEY_F4        135
#define KEY_F5        136
#define KEY_F6        137
#define KEY_F7        138
#define KEY_F8        139
#define KEY_F9        140
#define KEY_F10       141
#define KEY_F11       142
#define KEY_F12       143

/* Initialize keyboard */
void keyboard_init(void);

/* Poll for key (non-blocking, returns 0 if no key) */
char keyboard_getchar(void);

/* Check if a key is available */
int keyboard_haskey(void);

/* Get current modifier state */
uint8_t keyboard_get_modifiers(void);

/* Check if specific key is held */
int keyboard_key_held(uint8_t scancode);

/* Update keyboard state (call in main loop) */
void keyboard_update(void);

#endif /* KEYBOARD_H */
