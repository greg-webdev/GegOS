/*
 * gui.c - Simple GUI System for GegOS
 * Apple Lisa-inspired graphical user interface
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

/* Cursor backup for XOR drawing (no longer needed but kept for reference) */
static uint8_t cursor_backup[16 * 16] __attribute__((unused));
static int cursor_drawn = 0;
static int cursor_last_x = -1, cursor_last_y = -1;

/* Invalidate cursor backup - just mark as not drawn, XOR handles cleanup */
void gui_cursor_invalidate(void) {
    cursor_drawn = 0;
    cursor_last_x = -1;
    cursor_last_y = -1;
}

/* Mark window region as dirty */
void gui_mark_dirty(int window_id, int x, int y, int width, int height) {
    if (window_id < 0 || window_id >= num_windows) return;
    
    gui_window_t* win = &windows[window_id];
    if (!win->dirty_region.dirty) {
        win->dirty_region.x = x;
        win->dirty_region.y = y;
        win->dirty_region.width = width;
        win->dirty_region.height = height;
        win->dirty_region.dirty = 1;
    } else {
        /* Expand dirty region to include new area */
        int x2 = win->dirty_region.x + win->dirty_region.width;
        int y2 = win->dirty_region.y + win->dirty_region.height;
        int new_x2 = x + width;
        int new_y2 = y + height;
        
        if (x < win->dirty_region.x) win->dirty_region.x = x;
        if (y < win->dirty_region.y) win->dirty_region.y = y;
        if (new_x2 > x2) x2 = new_x2;
        if (new_y2 > y2) y2 = new_y2;
        
        win->dirty_region.width = x2 - win->dirty_region.x;
        win->dirty_region.height = y2 - win->dirty_region.y;
    }
}

/* Clear dirty flags */
void gui_clear_dirty(int window_id) {
    if (window_id < 0 || window_id >= num_windows) return;
    windows[window_id].dirty_region.dirty = 0;
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


/* Draw mouse cursor - bright yellow (always visible) */
void gui_draw_cursor(int x, int y) {
    /* Erase old cursor by restoring background color */
    if (cursor_drawn && cursor_last_x >= 0 && cursor_last_y >= 0) {
        int ox = cursor_last_x, oy = cursor_last_y;
        /* Draw with desktop background color to erase */
        uint8_t bg = get_desktop_color();
        vga_putpixel(ox, oy, bg);
        vga_putpixel(ox, oy+1, bg); vga_putpixel(ox+1, oy+1, bg);
        vga_putpixel(ox, oy+2, bg); vga_putpixel(ox+1, oy+2, bg); vga_putpixel(ox+2, oy+2, bg);
        vga_putpixel(ox, oy+3, bg); vga_putpixel(ox+1, oy+3, bg); vga_putpixel(ox+2, oy+3, bg); vga_putpixel(ox+3, oy+3, bg);
        vga_putpixel(ox, oy+4, bg); vga_putpixel(ox+1, oy+4, bg); vga_putpixel(ox+2, oy+4, bg); vga_putpixel(ox+3, oy+4, bg); vga_putpixel(ox+4, oy+4, bg);
        vga_putpixel(ox, oy+5, bg); vga_putpixel(ox+1, oy+5, bg); vga_putpixel(ox+2, oy+5, bg); vga_putpixel(ox+3, oy+5, bg); vga_putpixel(ox+4, oy+5, bg); vga_putpixel(ox+5, oy+5, bg);
        vga_putpixel(ox, oy+6, bg); vga_putpixel(ox+1, oy+6, bg); vga_putpixel(ox+2, oy+6, bg); vga_putpixel(ox+3, oy+6, bg); vga_putpixel(ox+4, oy+6, bg); vga_putpixel(ox+5, oy+6, bg); vga_putpixel(ox+6, oy+6, bg);
        vga_putpixel(ox, oy+7, bg); vga_putpixel(ox+1, oy+7, bg); vga_putpixel(ox+2, oy+7, bg); vga_putpixel(ox+3, oy+7, bg); vga_putpixel(ox+4, oy+7, bg); vga_putpixel(ox+5, oy+7, bg); vga_putpixel(ox+6, oy+7, bg);
        vga_putpixel(ox, oy+8, bg); vga_putpixel(ox+1, oy+8, bg); vga_putpixel(ox+2, oy+8, bg); vga_putpixel(ox+3, oy+8, bg); vga_putpixel(ox+4, oy+8, bg);
        vga_putpixel(ox, oy+9, bg); vga_putpixel(ox+1, oy+9, bg); vga_putpixel(ox+3, oy+9, bg); vga_putpixel(ox+4, oy+9, bg); vga_putpixel(ox+5, oy+9, bg);
        vga_putpixel(ox, oy+10, bg); vga_putpixel(ox+4, oy+10, bg); vga_putpixel(ox+5, oy+10, bg);
    }
    
    /* Clamp position */
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= SCREEN_WIDTH) x = SCREEN_WIDTH - 1;
    if (y >= SCREEN_HEIGHT) y = SCREEN_HEIGHT - 1;
    
    /* Draw new cursor at new position in bright yellow */
    vga_putpixel(x, y, COLOR_YELLOW);
    vga_putpixel(x, y+1, COLOR_YELLOW); vga_putpixel(x+1, y+1, COLOR_YELLOW);
    vga_putpixel(x, y+2, COLOR_YELLOW); vga_putpixel(x+1, y+2, COLOR_YELLOW); vga_putpixel(x+2, y+2, COLOR_YELLOW);
    vga_putpixel(x, y+3, COLOR_YELLOW); vga_putpixel(x+1, y+3, COLOR_YELLOW); vga_putpixel(x+2, y+3, COLOR_YELLOW); vga_putpixel(x+3, y+3, COLOR_YELLOW);
    vga_putpixel(x, y+4, COLOR_YELLOW); vga_putpixel(x+1, y+4, COLOR_YELLOW); vga_putpixel(x+2, y+4, COLOR_YELLOW); vga_putpixel(x+3, y+4, COLOR_YELLOW); vga_putpixel(x+4, y+4, COLOR_YELLOW);
    vga_putpixel(x, y+5, COLOR_YELLOW); vga_putpixel(x+1, y+5, COLOR_YELLOW); vga_putpixel(x+2, y+5, COLOR_YELLOW); vga_putpixel(x+3, y+5, COLOR_YELLOW); vga_putpixel(x+4, y+5, COLOR_YELLOW); vga_putpixel(x+5, y+5, COLOR_YELLOW);
    vga_putpixel(x, y+6, COLOR_YELLOW); vga_putpixel(x+1, y+6, COLOR_YELLOW); vga_putpixel(x+2, y+6, COLOR_YELLOW); vga_putpixel(x+3, y+6, COLOR_YELLOW); vga_putpixel(x+4, y+6, COLOR_YELLOW); vga_putpixel(x+5, y+6, COLOR_YELLOW); vga_putpixel(x+6, y+6, COLOR_YELLOW);
    vga_putpixel(x, y+7, COLOR_YELLOW); vga_putpixel(x+1, y+7, COLOR_YELLOW); vga_putpixel(x+2, y+7, COLOR_YELLOW); vga_putpixel(x+3, y+7, COLOR_YELLOW); vga_putpixel(x+4, y+7, COLOR_YELLOW); vga_putpixel(x+5, y+7, COLOR_YELLOW); vga_putpixel(x+6, y+7, COLOR_YELLOW);
    vga_putpixel(x, y+8, COLOR_YELLOW); vga_putpixel(x+1, y+8, COLOR_YELLOW); vga_putpixel(x+2, y+8, COLOR_YELLOW); vga_putpixel(x+3, y+8, COLOR_YELLOW); vga_putpixel(x+4, y+8, COLOR_YELLOW);
    vga_putpixel(x, y+9, COLOR_YELLOW); vga_putpixel(x+1, y+9, COLOR_YELLOW); vga_putpixel(x+3, y+9, COLOR_YELLOW); vga_putpixel(x+4, y+9, COLOR_YELLOW); vga_putpixel(x+5, y+9, COLOR_YELLOW);
    vga_putpixel(x, y+10, COLOR_YELLOW); vga_putpixel(x+4, y+10, COLOR_YELLOW); vga_putpixel(x+5, y+10, COLOR_YELLOW);
    
    cursor_drawn = 1;
    cursor_last_x = x;
    cursor_last_y = y;
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
