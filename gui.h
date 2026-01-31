/*
 * gui.h - Simple GUI System for GegOS
 * Apple Lisa-inspired graphical user interface
 */

#ifndef GUI_H
#define GUI_H

#include <stdint.h>

/* Maximum UI elements */
#define MAX_WINDOWS 8
#define MAX_BUTTONS 32

/* Window structure */
typedef struct {
    int x, y;
    int width, height;
    const char* title;
    int active;
    int dragging;
    int drag_offset_x, drag_offset_y;
    int visible;
} gui_window_t;

/* Button structure */
typedef struct {
    int x, y;
    int width, height;
    const char* label;
    int pressed;
    int hovered;
    void (*callback)(void);
    int visible;
    int window_id;  /* -1 for screen, else window index */
} gui_button_t;

/* Initialize GUI */
void gui_init(void);

/* Create a window, returns window ID */
int gui_create_window(int x, int y, int width, int height, const char* title);

/* Create a button, returns button ID */
int gui_create_button(int x, int y, int width, int height, const char* label, void (*callback)(void));

/* Create button inside a window */
int gui_create_window_button(int window_id, int x, int y, int width, int height, 
                              const char* label, void (*callback)(void));

/* Update GUI (handle input) */
void gui_update(void);

/* Draw entire GUI */
void gui_draw(void);

/* Draw desktop background */
void gui_draw_desktop(void);

/* Draw a window */
void gui_draw_window(gui_window_t* win);

/* Draw a button */
void gui_draw_button(gui_button_t* btn);

/* Draw mouse cursor */
void gui_draw_cursor(int x, int y);

/* Show/hide window */
void gui_show_window(int window_id, int visible);

/* Set active window */
void gui_set_active_window(int window_id);

/* Draw menubar */
void gui_draw_menubar(void);

/* Get window by ID */
gui_window_t* gui_get_window(int id);

/* Check if point is in rect */
int point_in_rect(int px, int py, int rx, int ry, int rw, int rh);

#endif /* GUI_H */
