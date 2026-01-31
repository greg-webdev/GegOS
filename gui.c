/*
 * gui.c - Simple GUI System for GegOS
 * Optimized with dirty rectangle system and proper cursor handling
 */

#include "gui.h"
#include "vga.h"
#include "mouse.h"
#include "keyboard.h"

/* GUI Colors (Windows 95/XP classic theme) */
#define GUI_COLOR_DESKTOP     COLOR_CYAN          /* Teal desktop */
#define GUI_COLOR_WINDOW_BG   COLOR_LIGHT_GRAY    /* Gray window background */
#define GUI_COLOR_WINDOW_FG   COLOR_BLACK
#define GUI_COLOR_TITLEBAR    COLOR_BLUE          /* Blue active titlebar */
#define GUI_COLOR_TITLE_TEXT  COLOR_WHITE
#define GUI_COLOR_BORDER      COLOR_BLACK
#define GUI_COLOR_BUTTON_BG   COLOR_LIGHT_GRAY    /* Gray buttons */
#define GUI_COLOR_BUTTON_FG   COLOR_BLACK
#define GUI_COLOR_BUTTON_HOVER COLOR_LIGHT_CYAN
#define GUI_COLOR_BUTTON_PRESS COLOR_DARK_GRAY
#define GUI_COLOR_TASKBAR     COLOR_LIGHT_GRAY    /* Gray taskbar */
#define GUI_COLOR_CURSOR      COLOR_WHITE

/* Window storage */
static gui_window_t windows[MAX_WINDOWS];
static int num_windows = 0;
static int active_window = -1;

/* Button storage */
static gui_button_t buttons[MAX_BUTTONS];
static int num_buttons = 0;

/* Cursor state - XOR based, no backup needed */
static int cursor_drawn = 0;
static int cursor_last_x = -1, cursor_last_y = -1;

/* Dirty rectangle system */
static dirty_rect_t dirty_rects[MAX_DIRTY_RECTS];
static int num_dirty_rects = 0;

/* Invalidate cursor - erase it first if drawn */
void gui_cursor_invalidate(void) {
    if (cursor_drawn) {
        gui_erase_cursor();
    }
    cursor_drawn = 0;
    cursor_last_x = -1;
    cursor_last_y = -1;
}

/* Add a dirty rectangle */
void gui_add_dirty_rect(int x, int y, int width, int height) {
    if (num_dirty_rects >= MAX_DIRTY_RECTS) {
        /* Overflow - mark whole screen dirty by merging all */
        dirty_rects[0].x = 0;
        dirty_rects[0].y = 0;
        dirty_rects[0].width = SCREEN_WIDTH;
        dirty_rects[0].height = SCREEN_HEIGHT;
        dirty_rects[0].dirty = 1;
        num_dirty_rects = 1;
        return;
    }
    
    /* Clamp to screen */
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > SCREEN_WIDTH) width = SCREEN_WIDTH - x;
    if (y + height > SCREEN_HEIGHT) height = SCREEN_HEIGHT - y;
    if (width <= 0 || height <= 0) return;
    
    dirty_rects[num_dirty_rects].x = x;
    dirty_rects[num_dirty_rects].y = y;
    dirty_rects[num_dirty_rects].width = width;
    dirty_rects[num_dirty_rects].height = height;
    dirty_rects[num_dirty_rects].dirty = 1;
    num_dirty_rects++;
}

/* Check if there are dirty rectangles */
int gui_has_dirty_rects(void) {
    return num_dirty_rects > 0;
}

/* Clear all dirty rectangles */
void gui_clear_dirty_rects(void) {
    num_dirty_rects = 0;
}

/* Redraw only dirty areas (stub - called from kernel) */
void gui_redraw_dirty(void) {
    /* This would be implemented with clipping, but for now kernel handles it */
    num_dirty_rects = 0;
}

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
    cursor_last_x = -1;
    cursor_last_y = -1;
    num_dirty_rects = 0;
    
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
    win->dirty_region.dirty = 0;
    
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

/* XOR a pixel - toggle between white and current color */
static void xor_pixel(int x, int y) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    uint8_t c = vga_getpixel(x, y);
    vga_putpixel(x, y, c ^ 0x0F);  /* XOR with white */
}

/* Draw XOR cursor at position */
static void draw_xor_cursor(int x, int y) {
    /* Simple arrow shape using XOR */
    xor_pixel(x, y);
    xor_pixel(x, y+1); xor_pixel(x+1, y+1);
    xor_pixel(x, y+2); xor_pixel(x+1, y+2); xor_pixel(x+2, y+2);
    xor_pixel(x, y+3); xor_pixel(x+1, y+3); xor_pixel(x+2, y+3); xor_pixel(x+3, y+3);
    xor_pixel(x, y+4); xor_pixel(x+1, y+4); xor_pixel(x+2, y+4); xor_pixel(x+3, y+4); xor_pixel(x+4, y+4);
    xor_pixel(x, y+5); xor_pixel(x+1, y+5); xor_pixel(x+2, y+5); xor_pixel(x+3, y+5); xor_pixel(x+4, y+5); xor_pixel(x+5, y+5);
    xor_pixel(x, y+6); xor_pixel(x+1, y+6); xor_pixel(x+2, y+6); xor_pixel(x+3, y+6);
    xor_pixel(x, y+7); xor_pixel(x+1, y+7); xor_pixel(x+3, y+7); xor_pixel(x+4, y+7);
    xor_pixel(x, y+8); xor_pixel(x+4, y+8); xor_pixel(x+5, y+8);
    xor_pixel(x+5, y+9); xor_pixel(x+6, y+9);
}

/* Erase cursor by XORing again at same position */
void gui_erase_cursor(void) {
    if (!cursor_drawn || cursor_last_x < 0 || cursor_last_y < 0) return;
    draw_xor_cursor(cursor_last_x, cursor_last_y);
    cursor_drawn = 0;
}

/* Draw mouse cursor with background saving */
void gui_draw_cursor(int x, int y) {
    /* Erase old cursor first */
    gui_erase_cursor();
    
    /* Clamp position */
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= SCREEN_WIDTH - 1) x = SCREEN_WIDTH - 2;
    if (y >= SCREEN_HEIGHT - 1) y = SCREEN_HEIGHT - 2;
    
    /* Save background under new cursor position */
    for (int j = 0; j < CURSOR_HEIGHT; j++) {
        for (int i = 0; i < CURSOR_WIDTH; i++) {
            int px = x + i;
            int py = y + j;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                cursor_backup[j * CURSOR_WIDTH + i] = vga_getpixel(px, py);
            } else {
                cursor_backup[j * CURSOR_WIDTH + i] = 0;
            }
        }
    }
    cursor_backup_valid = 1;
    cursor_last_x = x;
    cursor_last_y = y;
    
    /* Sample center for smart color */
    uint8_t bg = vga_getpixel(x + 3, y + 3);
    uint8_t cursor_color = (bg > 7) ? COLOR_BLACK : COLOR_WHITE;
    
    /* Draw arrow cursor shape */
    /* Row 0 */
    vga_putpixel(x, y, cursor_color);
    /* Row 1 */
    vga_putpixel(x, y+1, cursor_color); vga_putpixel(x+1, y+1, cursor_color);
    /* Row 2 */
    vga_putpixel(x, y+2, cursor_color); vga_putpixel(x+1, y+2, cursor_color); vga_putpixel(x+2, y+2, cursor_color);
    /* Row 3 */
    vga_putpixel(x, y+3, cursor_color); vga_putpixel(x+1, y+3, cursor_color); vga_putpixel(x+2, y+3, cursor_color); vga_putpixel(x+3, y+3, cursor_color);
    /* Row 4 */
    vga_putpixel(x, y+4, cursor_color); vga_putpixel(x+1, y+4, cursor_color); vga_putpixel(x+2, y+4, cursor_color); vga_putpixel(x+3, y+4, cursor_color); vga_putpixel(x+4, y+4, cursor_color);
    /* Row 5 */
    vga_putpixel(x, y+5, cursor_color); vga_putpixel(x+1, y+5, cursor_color); vga_putpixel(x+2, y+5, cursor_color); vga_putpixel(x+3, y+5, cursor_color); vga_putpixel(x+4, y+5, cursor_color); vga_putpixel(x+5, y+5, cursor_color);
    /* Row 6 */
    vga_putpixel(x, y+6, cursor_color); vga_putpixel(x+1, y+6, cursor_color); vga_putpixel(x+2, y+6, cursor_color); vga_putpixel(x+3, y+6, cursor_color); vga_putpixel(x+4, y+6, cursor_color); vga_putpixel(x+5, y+6, cursor_color); vga_putpixel(x+6, y+6, cursor_color);
    /* Row 7 - narrowing */
    vga_putpixel(x, y+7, cursor_color); vga_putpixel(x+1, y+7, cursor_color); vga_putpixel(x+2, y+7, cursor_color); vga_putpixel(x+3, y+7, cursor_color); vga_putpixel(x+4, y+7, cursor_color);
    /* Row 8 */
    vga_putpixel(x, y+8, cursor_color); vga_putpixel(x+1, y+8, cursor_color); vga_putpixel(x+3, y+8, cursor_color); vga_putpixel(x+4, y+8, cursor_color);
    /* Row 9 */
    vga_putpixel(x, y+9, cursor_color); vga_putpixel(x+4, y+9, cursor_color); vga_putpixel(x+5, y+9, cursor_color);
    /* Row 10 */
    vga_putpixel(x+5, y+10, cursor_color);
}

/* Draw Windows-style taskbar */
void gui_draw_menubar(void) {
    int taskbar_height = 28;
    int taskbar_y = SCREEN_HEIGHT - taskbar_height;
    
    /* Taskbar background */
    vga_fillrect(0, taskbar_y, SCREEN_WIDTH, taskbar_height, GUI_COLOR_TASKBAR);
    
    /* Top border (raised) */
    vga_hline(0, taskbar_y, SCREEN_WIDTH, COLOR_WHITE);
    vga_hline(0, taskbar_y + 1, SCREEN_WIDTH, COLOR_WHITE);
    
    /* Start button (Windows style) */
    int start_w = 60;
    int start_h = 22;
    int start_x = 2;
    int start_y = taskbar_y + 3;
    
    /* Start button 3D raised border */
    vga_fillrect(start_x, start_y, start_w, start_h, GUI_COLOR_BUTTON_BG);
    vga_hline(start_x, start_y, start_w, COLOR_WHITE);
    vga_vline(start_x, start_y, start_h, COLOR_WHITE);
    vga_hline(start_x, start_y + start_h - 1, start_w, COLOR_BLACK);
    vga_vline(start_x + start_w - 1, start_y, start_h, COLOR_BLACK);
    vga_hline(start_x + 1, start_y + start_h - 2, start_w - 2, COLOR_DARK_GRAY);
    vga_vline(start_x + start_w - 2, start_y + 1, start_h - 2, COLOR_DARK_GRAY);
    
    /* Windows logo (simplified) */
    vga_fillrect(start_x + 5, start_y + 5, 4, 5, COLOR_RED);
    vga_fillrect(start_x + 5, start_y + 11, 4, 5, COLOR_RED);
    vga_fillrect(start_x + 10, start_y + 5, 4, 5, COLOR_RED);
    vga_fillrect(start_x + 10, start_y + 11, 4, 5, COLOR_RED);
    
    /* Start text */
    vga_putstring(start_x + 18, start_y + 7, "Start", COLOR_BLACK, GUI_COLOR_BUTTON_BG);
    
    /* Clock area (sunken) */
    int clock_x = SCREEN_WIDTH - 60;
    vga_fillrect(clock_x, start_y, 56, start_h, GUI_COLOR_TASKBAR);
    vga_hline(clock_x, start_y, 56, COLOR_DARK_GRAY);
    vga_vline(clock_x, start_y, start_h, COLOR_DARK_GRAY);
    vga_hline(clock_x + 1, start_y + 1, 54, COLOR_BLACK);
    vga_vline(clock_x + 1, start_y + 1, start_h - 2, COLOR_BLACK);
    vga_hline(clock_x, start_y + start_h - 1, 56, COLOR_WHITE);
    vga_vline(clock_x + 55, start_y, start_h, COLOR_WHITE);
    vga_putstring(clock_x + 8, start_y + 7, "12:00 PM", COLOR_BLACK, GUI_COLOR_TASKBAR);
}

/* Draw desktop */
void gui_draw_desktop(void) {
    /* Windows teal/cyan desktop */
    vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - 28, GUI_COLOR_DESKTOP);
}

/* Draw window */
void gui_draw_window(gui_window_t* win) {
    if (!win->visible) return;
    
    int x = win->x;
    int y = win->y;
    int w = win->width;
    int h = win->height;
    
    /* Outer 3D border (raised) */
    vga_fillrect(x, y, w, h, GUI_COLOR_WINDOW_BG);
    
    /* Light edges (top-left) */
    vga_hline(x, y, w, COLOR_WHITE);
    vga_vline(x, y, h, COLOR_WHITE);
    vga_hline(x + 1, y + 1, w - 2, COLOR_WHITE);
    vga_vline(x + 1, y + 1, h - 2, COLOR_WHITE);
    
    /* Dark edges (bottom-right) */
    vga_hline(x, y + h - 1, w, COLOR_BLACK);
    vga_vline(x + w - 1, y, h, COLOR_BLACK);
    vga_hline(x + 1, y + h - 2, w - 2, COLOR_DARK_GRAY);
    vga_vline(x + w - 2, y + 1, h - 2, COLOR_DARK_GRAY);
    
    /* Title bar */
    uint8_t titlebar_color = win->active ? GUI_COLOR_TITLEBAR : COLOR_DARK_GRAY;
    vga_fillrect(x + 3, y + 3, w - 6, 18, titlebar_color);
    
    /* Gradient effect (lighter at top) */
    if (win->active) {
        vga_hline(x + 3, y + 3, w - 6, COLOR_LIGHT_BLUE);
        vga_hline(x + 3, y + 4, w - 6, COLOR_LIGHT_BLUE);
    }
    
    /* Title text */
    if (win->title) {
        vga_putstring(x + 8, y + 7, win->title, GUI_COLOR_TITLE_TEXT, titlebar_color);
    }
    
    /* Window control buttons */
    int btn_size = 14;
    int btn_y = win->y + 5;
    
    /* Close button (X) */
    int close_x = win->x + win->width - btn_size - 6;
    vga_fillrect(close_x, btn_y, btn_size, btn_size, COLOR_LIGHT_GRAY);
    vga_hline(close_x, btn_y, btn_size, COLOR_WHITE);
    vga_vline(close_x, btn_y, btn_size, COLOR_WHITE);
    vga_hline(close_x, btn_y + btn_size - 1, btn_size, COLOR_BLACK);
    vga_vline(close_x + btn_size - 1, btn_y, btn_size, COLOR_BLACK);
    vga_line(close_x + 3, btn_y + 3, close_x + 10, btn_y + 10, COLOR_BLACK);
    vga_line(close_x + 10, btn_y + 3, close_x + 3, btn_y + 10, COLOR_BLACK);
    
    /* Maximize button */
    int max_x = close_x - btn_size - 2;
    vga_fillrect(max_x, btn_y, btn_size, btn_size, COLOR_LIGHT_GRAY);
    vga_hline(max_x, btn_y, btn_size, COLOR_WHITE);
    vga_vline(max_x, btn_y, btn_size, COLOR_WHITE);
    vga_hline(max_x, btn_y + btn_size - 1, btn_size, COLOR_BLACK);
    vga_vline(max_x + btn_size - 1, btn_y, btn_size, COLOR_BLACK);
    vga_rect(max_x + 3, btn_y + 3, 7, 7, COLOR_BLACK);
    vga_hline(max_x + 3, btn_y + 3, 7, COLOR_BLACK);
    
    /* Minimize button */
    int min_x = max_x - btn_size - 2;
    vga_fillrect(min_x, btn_y, btn_size, btn_size, COLOR_LIGHT_GRAY);
    vga_hline(min_x, btn_y, btn_size, COLOR_WHITE);
    vga_vline(min_x, btn_y, btn_size, COLOR_WHITE);
    vga_hline(min_x, btn_y + btn_size - 1, btn_size, COLOR_BLACK);
    vga_vline(min_x + btn_size - 1, btn_y, btn_size, COLOR_BLACK);
    vga_hline(min_x + 3, btn_y + 10, 7, COLOR_BLACK);
    vga_hline(min_x + 3, btn_y + 11, 7, COLOR_BLACK);
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
