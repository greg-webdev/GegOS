/*
 * gui.c - Simple GUI System for GegOS
 * Apple Lisa-inspired graphical user interface
 */

#include "gui.h"
#include "vga.h"
#include "mouse.h"
#include "keyboard.h"

/* GUI Colors (Lisa-inspired grayscale palette) */
#define GUI_COLOR_DESKTOP     COLOR_LIGHT_GRAY
#define GUI_COLOR_WINDOW_BG   COLOR_WHITE
#define GUI_COLOR_WINDOW_FG   COLOR_BLACK
#define GUI_COLOR_TITLEBAR    COLOR_BLACK
#define GUI_COLOR_TITLE_TEXT  COLOR_WHITE
#define GUI_COLOR_BORDER      COLOR_BLACK
#define GUI_COLOR_BUTTON_BG   COLOR_WHITE
#define GUI_COLOR_BUTTON_FG   COLOR_BLACK
#define GUI_COLOR_BUTTON_HOVER COLOR_LIGHT_GRAY
#define GUI_COLOR_BUTTON_PRESS COLOR_DARK_GRAY
#define GUI_COLOR_MENUBAR     COLOR_WHITE
#define GUI_COLOR_CURSOR      COLOR_BLACK

/* Window storage */
static gui_window_t windows[MAX_WINDOWS];
static int num_windows = 0;
static int active_window = -1;

/* Button storage */
static gui_button_t buttons[MAX_BUTTONS];
static int num_buttons = 0;

/* Cursor backup for XOR drawing */
static uint8_t cursor_backup[16 * 16];
static int cursor_drawn = 0;
static int cursor_last_x = -1, cursor_last_y = -1;

/* Point in rect check */
int point_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

/* Initialize GUI */
void gui_init(void) {
    num_windows = 0;
    num_buttons = 0;
    active_window = -1;
    cursor_drawn = 0;
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].visible = 0;
        windows[i].active = 0;
    }
    
    for (int i = 0; i < MAX_BUTTONS; i++) {
        buttons[i].visible = 0;
    }
}

/* Create window */
int gui_create_window(int x, int y, int width, int height, const char* title) {
    if (num_windows >= MAX_WINDOWS) return -1;
    
    int id = num_windows++;
    gui_window_t* win = &windows[id];
    
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->title = title;
    win->active = 0;
    win->dragging = 0;
    win->visible = 1;
    
    return id;
}

/* Create button */
int gui_create_button(int x, int y, int width, int height, const char* label, void (*callback)(void)) {
    if (num_buttons >= MAX_BUTTONS) return -1;
    
    int id = num_buttons++;
    gui_button_t* btn = &buttons[id];
    
    btn->x = x;
    btn->y = y;
    btn->width = width;
    btn->height = height;
    btn->label = label;
    btn->callback = callback;
    btn->pressed = 0;
    btn->hovered = 0;
    btn->visible = 1;
    btn->window_id = -1;
    
    return id;
}

/* Create button in window */
int gui_create_window_button(int window_id, int x, int y, int width, int height,
                              const char* label, void (*callback)(void)) {
    int id = gui_create_button(x, y, width, height, label, callback);
    if (id >= 0) {
        buttons[id].window_id = window_id;
    }
    return id;
}

/* Get window */
gui_window_t* gui_get_window(int id) {
    if (id >= 0 && id < num_windows) {
        return &windows[id];
    }
    return 0;
}

/* Show/hide window */
void gui_show_window(int window_id, int visible) {
    if (window_id >= 0 && window_id < num_windows) {
        windows[window_id].visible = visible;
    }
}

/* Set active window */
void gui_set_active_window(int window_id) {
    for (int i = 0; i < num_windows; i++) {
        windows[i].active = (i == window_id);
    }
    active_window = window_id;
}

/* Save cursor background */
/* Save cursor background */
static void cursor_save_bg(int x, int y) {
    for (int dy = 0; dy < 12; dy++) {
        for (int dx = 0; dx < 12; dx++) {
            if (x + dx < SCREEN_WIDTH && y + dy < SCREEN_HEIGHT && x + dx >= 0 && y + dy >= 0) {
                cursor_backup[dy * 16 + dx] = vga_getpixel(x + dx, y + dy);
            }
        }
    }
}

/* Restore cursor background */
static void cursor_restore_bg(int x, int y) {
    for (int dy = 0; dy < 12; dy++) {
        for (int dx = 0; dx < 12; dx++) {
            if (x + dx < SCREEN_WIDTH && y + dy < SCREEN_HEIGHT && x + dx >= 0 && y + dy >= 0) {
                vga_putpixel(x + dx, y + dy, cursor_backup[dy * 16 + dx]);
            }
        }
    }
}

/* Draw mouse cursor - simplified arrow */
void gui_draw_cursor(int x, int y) {
    /* Restore previous position first */
    if (cursor_drawn) {
        cursor_restore_bg(cursor_last_x, cursor_last_y);
    }
    
    /* Clamp position */
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= SCREEN_WIDTH) x = SCREEN_WIDTH - 1;
    if (y >= SCREEN_HEIGHT) y = SCREEN_HEIGHT - 1;
    
    /* Save new background */
    cursor_save_bg(x, y);
    
    /* Draw simple arrow cursor (smaller, 12x12) */
    /* Row 0 */  vga_putpixel(x, y, COLOR_BLACK);
    /* Row 1 */  vga_putpixel(x, y+1, COLOR_BLACK); vga_putpixel(x+1, y+1, COLOR_BLACK);
    /* Row 2 */  vga_putpixel(x, y+2, COLOR_BLACK); vga_putpixel(x+1, y+2, COLOR_WHITE); vga_putpixel(x+2, y+2, COLOR_BLACK);
    /* Row 3 */  vga_putpixel(x, y+3, COLOR_BLACK); vga_putpixel(x+1, y+3, COLOR_WHITE); vga_putpixel(x+2, y+3, COLOR_WHITE); vga_putpixel(x+3, y+3, COLOR_BLACK);
    /* Row 4 */  vga_putpixel(x, y+4, COLOR_BLACK); vga_putpixel(x+1, y+4, COLOR_WHITE); vga_putpixel(x+2, y+4, COLOR_WHITE); vga_putpixel(x+3, y+4, COLOR_WHITE); vga_putpixel(x+4, y+4, COLOR_BLACK);
    /* Row 5 */  vga_putpixel(x, y+5, COLOR_BLACK); vga_putpixel(x+1, y+5, COLOR_WHITE); vga_putpixel(x+2, y+5, COLOR_WHITE); vga_putpixel(x+3, y+5, COLOR_WHITE); vga_putpixel(x+4, y+5, COLOR_WHITE); vga_putpixel(x+5, y+5, COLOR_BLACK);
    /* Row 6 */  vga_putpixel(x, y+6, COLOR_BLACK); vga_putpixel(x+1, y+6, COLOR_WHITE); vga_putpixel(x+2, y+6, COLOR_WHITE); vga_putpixel(x+3, y+6, COLOR_WHITE); vga_putpixel(x+4, y+6, COLOR_WHITE); vga_putpixel(x+5, y+6, COLOR_WHITE); vga_putpixel(x+6, y+6, COLOR_BLACK);
    /* Row 7 */  vga_putpixel(x, y+7, COLOR_BLACK); vga_putpixel(x+1, y+7, COLOR_WHITE); vga_putpixel(x+2, y+7, COLOR_WHITE); vga_putpixel(x+3, y+7, COLOR_WHITE); vga_putpixel(x+4, y+7, COLOR_BLACK); vga_putpixel(x+5, y+7, COLOR_BLACK); vga_putpixel(x+6, y+7, COLOR_BLACK);
    /* Row 8 */  vga_putpixel(x, y+8, COLOR_BLACK); vga_putpixel(x+1, y+8, COLOR_WHITE); vga_putpixel(x+2, y+8, COLOR_BLACK); vga_putpixel(x+3, y+8, COLOR_WHITE); vga_putpixel(x+4, y+8, COLOR_BLACK);
    /* Row 9 */  vga_putpixel(x, y+9, COLOR_BLACK); vga_putpixel(x+1, y+9, COLOR_BLACK); vga_putpixel(x+3, y+9, COLOR_BLACK); vga_putpixel(x+4, y+9, COLOR_WHITE); vga_putpixel(x+5, y+9, COLOR_BLACK);
    /* Row 10 */ vga_putpixel(x, y+10, COLOR_BLACK); vga_putpixel(x+4, y+10, COLOR_BLACK); vga_putpixel(x+5, y+10, COLOR_BLACK);
    
    cursor_drawn = 1;
    cursor_last_x = x;
    cursor_last_y = y;
}

/* Draw menubar */
void gui_draw_menubar(void) {
    /* Menu bar background */
    vga_fillrect(0, 0, SCREEN_WIDTH, 12, GUI_COLOR_MENUBAR);
    vga_hline(0, 12, SCREEN_WIDTH, GUI_COLOR_BORDER);
    
    /* Apple/System menu icon (simple) */
    vga_fillrect(4, 2, 8, 8, COLOR_BLACK);
    
    /* Menu items */
    vga_putstring(16, 2, "File", COLOR_BLACK, GUI_COLOR_MENUBAR);
    vga_putstring(48, 2, "Edit", COLOR_BLACK, GUI_COLOR_MENUBAR);
    vga_putstring(80, 2, "View", COLOR_BLACK, GUI_COLOR_MENUBAR);
    vga_putstring(112, 2, "Help", COLOR_BLACK, GUI_COLOR_MENUBAR);
}

/* Draw desktop */
void gui_draw_desktop(void) {
    /* Simple solid desktop - much faster than pattern */
    vga_fillrect(0, 13, SCREEN_WIDTH, SCREEN_HEIGHT - 13, COLOR_CYAN);
}

/* Draw window */
void gui_draw_window(gui_window_t* win) {
    if (!win->visible) return;
    
    int x = win->x;
    int y = win->y;
    int w = win->width;
    int h = win->height;
    
    /* Window shadow */
    vga_fillrect(x + 2, y + 2, w, h, COLOR_DARK_GRAY);
    
    /* Window background */
    vga_fillrect(x, y, w, h, GUI_COLOR_WINDOW_BG);
    
    /* Window border */
    vga_rect(x, y, w, h, GUI_COLOR_BORDER);
    vga_rect(x + 1, y + 1, w - 2, h - 2, GUI_COLOR_BORDER);
    
    /* Title bar */
    if (win->active) {
        /* Active window - filled title bar */
        vga_fillrect(x + 2, y + 2, w - 4, 12, GUI_COLOR_TITLEBAR);
        vga_putstring(x + 6, y + 4, win->title, GUI_COLOR_TITLE_TEXT, GUI_COLOR_TITLEBAR);
    } else {
        /* Inactive window - striped title bar */
        for (int ty = y + 2; ty < y + 14; ty += 2) {
            vga_hline(x + 2, ty, w - 4, GUI_COLOR_BORDER);
        }
        vga_fillrect(x + 6, y + 3, 8 * 10, 10, GUI_COLOR_WINDOW_BG);
        vga_putstring(x + 6, y + 4, win->title, GUI_COLOR_WINDOW_FG, GUI_COLOR_WINDOW_BG);
    }
    
    /* Title bar separator */
    vga_hline(x + 2, y + 14, w - 4, GUI_COLOR_BORDER);
    
    /* Close box with X */
    vga_rect(x + w - 14, y + 3, 10, 10, GUI_COLOR_BORDER);
    vga_fillrect(x + w - 13, y + 4, 8, 8, GUI_COLOR_WINDOW_BG);
    /* Draw X */
    vga_line(x + w - 12, y + 5, x + w - 7, y + 10, GUI_COLOR_BORDER);
    vga_line(x + w - 12, y + 10, x + w - 7, y + 5, GUI_COLOR_BORDER);
}

/* Draw button */
void gui_draw_button(gui_button_t* btn) {
    if (!btn->visible) return;
    
    int x = btn->x;
    int y = btn->y;
    
    /* Offset for window-relative buttons */
    if (btn->window_id >= 0 && btn->window_id < num_windows) {
        gui_window_t* win = &windows[btn->window_id];
        if (!win->visible) return;
        x += win->x;
        y += win->y + 16;  /* Below title bar */
    }
    
    uint8_t bg_color = GUI_COLOR_BUTTON_BG;
    if (btn->pressed) {
        bg_color = GUI_COLOR_BUTTON_PRESS;
    } else if (btn->hovered) {
        bg_color = GUI_COLOR_BUTTON_HOVER;
    }
    
    /* Button background */
    vga_fillrect(x, y, btn->width, btn->height, bg_color);
    
    /* Button border */
    vga_rect(x, y, btn->width, btn->height, GUI_COLOR_BORDER);
    
    /* Button shadow (3D effect) */
    if (!btn->pressed) {
        vga_hline(x + 1, y + 1, btn->width - 2, COLOR_WHITE);
        vga_vline(x + 1, y + 1, btn->height - 2, COLOR_WHITE);
        vga_hline(x + 1, y + btn->height - 2, btn->width - 2, COLOR_DARK_GRAY);
        vga_vline(x + btn->width - 2, y + 1, btn->height - 2, COLOR_DARK_GRAY);
    } else {
        vga_hline(x + 1, y + 1, btn->width - 2, COLOR_DARK_GRAY);
        vga_vline(x + 1, y + 1, btn->height - 2, COLOR_DARK_GRAY);
    }
    
    /* Calculate text position (centered) */
    int text_len = 0;
    const char* s = btn->label;
    while (*s++) text_len++;
    
    int text_x = x + (btn->width - text_len * 8) / 2;
    int text_y = y + (btn->height - 8) / 2;
    
    if (btn->pressed) {
        text_x++;
        text_y++;
    }
    
    vga_putstring(text_x, text_y, btn->label, GUI_COLOR_BUTTON_FG, bg_color);
}

/* Update GUI */
void gui_update(void) {
    int mx = mouse_get_x();
    int my = mouse_get_y();
    int clicked = mouse_button_clicked(MOUSE_LEFT);
    int down = mouse_button_down(MOUSE_LEFT);
    int released = mouse_button_released(MOUSE_LEFT);
    
    /* Handle window dragging */
    for (int i = num_windows - 1; i >= 0; i--) {
        gui_window_t* win = &windows[i];
        if (!win->visible) continue;
        
        if (win->dragging) {
            if (down) {
                win->x = mx - win->drag_offset_x;
                win->y = my - win->drag_offset_y;
                
                /* Keep on screen */
                if (win->x < 0) win->x = 0;
                if (win->y < 13) win->y = 13;
                if (win->x + win->width > SCREEN_WIDTH) 
                    win->x = SCREEN_WIDTH - win->width;
                if (win->y + win->height > SCREEN_HEIGHT)
                    win->y = SCREEN_HEIGHT - win->height;
            } else {
                win->dragging = 0;
            }
            return;
        }
    }
    
    /* Check for window clicks */
    for (int i = num_windows - 1; i >= 0; i--) {
        gui_window_t* win = &windows[i];
        if (!win->visible) continue;
        
        /* Check close button */
        if (clicked && point_in_rect(mx, my, win->x + win->width - 14, win->y + 3, 10, 10)) {
            win->visible = 0;
            return;
        }
        
        /* Check title bar for dragging */
        if (clicked && point_in_rect(mx, my, win->x, win->y, win->width, 16)) {
            win->dragging = 1;
            win->drag_offset_x = mx - win->x;
            win->drag_offset_y = my - win->y;
            gui_set_active_window(i);
            return;
        }
        
        /* Check window body click to activate */
        if (clicked && point_in_rect(mx, my, win->x, win->y, win->width, win->height)) {
            gui_set_active_window(i);
        }
    }
    
    /* Update buttons */
    for (int i = 0; i < num_buttons; i++) {
        gui_button_t* btn = &buttons[i];
        if (!btn->visible) continue;
        
        int bx = btn->x;
        int by = btn->y;
        
        if (btn->window_id >= 0 && btn->window_id < num_windows) {
            gui_window_t* win = &windows[btn->window_id];
            if (!win->visible) continue;
            bx += win->x;
            by += win->y + 16;
        }
        
        int hover = point_in_rect(mx, my, bx, by, btn->width, btn->height);
        btn->hovered = hover;
        
        if (hover) {
            if (clicked) {
                btn->pressed = 1;
            }
            if (released && btn->pressed) {
                btn->pressed = 0;
                if (btn->callback) {
                    btn->callback();
                }
            }
        } else {
            btn->pressed = 0;
        }
    }
}

/* Draw entire GUI (windows only, no desktop/menubar/cursor - kernel handles those) */
void gui_draw(void) {
    /* Draw windows (back to front) */
    for (int i = 0; i < num_windows; i++) {
        if (i != active_window) {
            gui_draw_window(&windows[i]);
        }
    }
    
    /* Draw active window last (on top) */
    if (active_window >= 0 && active_window < num_windows) {
        gui_draw_window(&windows[active_window]);
    }
    
    /* Draw buttons */
    for (int i = 0; i < num_buttons; i++) {
        gui_draw_button(&buttons[i]);
    }
}
