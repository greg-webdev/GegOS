/*
 * mouse.h - PS/2 Mouse Driver for GegOS
 */

#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

/* Mouse button flags */
#define MOUSE_LEFT   (1 << 0)
#define MOUSE_RIGHT  (1 << 1)
#define MOUSE_MIDDLE (1 << 2)

/* Mouse state structure */
typedef struct {
    int x;
    int y;
    int dx;
    int dy;
    uint8_t buttons;
    uint8_t prev_buttons;
} mouse_state_t;

/* Initialize mouse */
void mouse_init(void);

/* Update mouse state (call in main loop) */
void mouse_update(void);

/* Get current mouse state */
mouse_state_t* mouse_get_state(void);

/* Get mouse X position */
int mouse_get_x(void);

/* Get mouse Y position */
int mouse_get_y(void);

/* Check if button is currently pressed */
int mouse_button_down(uint8_t button);

/* Check if button was just clicked (pressed this frame) */
int mouse_button_clicked(uint8_t button);

/* Check if button was just released */
int mouse_button_released(uint8_t button);

/* Set mouse position */
void mouse_set_position(int x, int y);

/* Set mouse bounds */
void mouse_set_bounds(int min_x, int min_y, int max_x, int max_y);

#endif /* MOUSE_H */
