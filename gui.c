/*
 * gui.c - GUI System 2.0 for GegOS
 * Complete rewrite with proper cursor handling
 * 
 * Key changes in 2.0:
 * - Cursor drawn LAST after all rendering
 * - No save/restore needed - full screen redraws each frame
 * - Simpler, more reliable rendering pipeline
 */

#include "gui.h"
#include "vga.h"
#include "mouse.h"
#include "keyboard.h"
#include "io.h"

/* External declarations */
extern void redraw_cursor_area_kernel(int x, int y);

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

/* Window storage */
static gui_window_t windows[MAX_WINDOWS];
static int num_windows = 0;
static int active_window = -1;

/* Button storage */
static gui_button_t buttons[MAX_BUTTONS];
static int num_buttons = 0;

/* ============================================================================
 * CURSOR SYSTEM 2.0 - Optimized rectangle redraws
 * ============================================================================ */

/* Cursor state */
static int cursor_visible = 0;
static int cursor_x = 0;
static int cursor_y = 0;
static int cursor_last_x = -1;
static int cursor_last_y = -1;

/* Cursor dimensions */
#define CURSOR_WIDTH 12
#define CURSOR_BUFFER 32  /* Extra pixels to redraw around cursor */

/* Arrow cursor shape - 1=black border, 2=white fill, 0=transparent */
static const uint8_t cursor_shape[16][12] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0},
    {1,2,2,2,2,2,1,1,1,1,1,0},
    {1,2,2,1,2,2,1,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0},
    {1,1,0,0,1,2,2,1,0,0,0,0},
    {1,0,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,1,1,1,1,0,0,0}
};

/* Redraw the area around a cursor position (cursor + buffer) */
static void redraw_cursor_area(int x, int y) {
    /* Calculate redraw rectangle */
    int left = x - CURSOR_BUFFER;
    int top = y - CURSOR_BUFFER;
    int width = CURSOR_WIDTH + CURSOR_BUFFER * 2;
    int height = CURSOR_HEIGHT + CURSOR_BUFFER * 2;
    
    /* Clamp to screen bounds */
    if (left < 0) {
        width += left;
        left = 0;
    }
    if (top < 0) {
        height += top;
        top = 0;
    }
    if (left + width > SCREEN_WIDTH) {
        width = SCREEN_WIDTH - left;
    }
    if (top + height > SCREEN_HEIGHT) {
        height = SCREEN_HEIGHT - top;
    }
    
    if (width <= 0 || height <= 0) return;
    
    /* Redraw desktop background in this area */
    vga_fillrect(left, top, width, height, get_desktop_color());
    
    /* Redraw any desktop icons that intersect this area */
    for (int i = 0; desktop_icons[i].label; i++) {
        int ix = desktop_icons[i].x;
        int iy = desktop_icons[i].y;
        int iw = 48;
        int ih = 32;
        
        /* Check if icon intersects redraw area */
        if (ix < left + width && ix + iw > left &&
            iy < top + height && iy + ih > top) {
            
            /* Redraw the icon */
            vga_fillrect(ix, iy, iw, ih, COLOR_WHITE);
            vga_rect(ix, iy, iw, ih, COLOR_BLACK);
            vga_fillrect(ix + 14, iy + 4, 20, 16, COLOR_BLUE);
            
            int label_len = 0;
            const char* s = desktop_icons[i].label;
            while (*s++) label_len++;
            int lx = ix + (iw - label_len * 8) / 2;
            vga_putstring(lx, iy + 23, desktop_icons[i].label, COLOR_BLACK, COLOR_WHITE);
        }
    }
    
    /* Redraw taskbar if it intersects */
    int taskbar_y = SCREEN_HEIGHT - 28;
    if (top + height > taskbar_y) {
        /* Redraw taskbar in the intersecting area */
        int tb_left = left;
        int tb_width = width;
        if (tb_left < 0) tb_left = 0;
        if (tb_left + tb_width > SCREEN_WIDTH) tb_width = SCREEN_WIDTH - tb_left;
        
        vga_fillrect(tb_left, taskbar_y, tb_width, 28, GUI_COLOR_TASKBAR);
        
        /* Redraw taskbar elements if they intersect */
        if (tb_left < 60) {  /* Start button */
            int start_w = 60;
            int start_h = 22;
            int start_x = 2;
            int start_y = taskbar_y + 3;
            
            vga_fillrect(start_x, start_y, start_w, start_h, GUI_COLOR_BUTTON_BG);
            vga_hline(start_x, start_y, start_w, COLOR_WHITE);
            vga_vline(start_x, start_y, start_h, COLOR_WHITE);
            vga_hline(start_x, start_y + start_h - 1, start_w, COLOR_BLACK);
            vga_vline(start_x + start_w - 1, start_y, start_h, COLOR_BLACK);
            vga_hline(start_x + 1, start_y + start_h - 2, start_w - 2, COLOR_DARK_GRAY);
            vga_vline(start_x + start_w - 2, start_y + 1, start_h - 2, COLOR_DARK_GRAY);
            
            vga_fillrect(start_x + 5, start_y + 5, 5, 5, COLOR_RED);
            vga_fillrect(start_x + 5, start_y + 11, 5, 5, COLOR_BLUE);
            vga_fillrect(start_x + 11, start_y + 5, 5, 5, COLOR_GREEN);
            vga_fillrect(start_x + 11, start_y + 11, 5, 5, COLOR_YELLOW);
            
            vga_putstring(start_x + 20, start_y + 7, "Start", COLOR_BLACK, GUI_COLOR_BUTTON_BG);
        }
        
        if (tb_left + tb_width > SCREEN_WIDTH - 60) {  /* Clock */
            int clock_x = SCREEN_WIDTH - 60;
            int start_y = taskbar_y + 3;
            int start_h = 22;
            
            vga_fillrect(clock_x, start_y, 56, start_h, GUI_COLOR_TASKBAR);
            vga_hline(clock_x, start_y, 56, COLOR_DARK_GRAY);
            vga_vline(clock_x, start_y, start_h, COLOR_DARK_GRAY);
            vga_hline(clock_x + 1, start_y + 1, 54, COLOR_BLACK);
            vga_vline(clock_x + 1, start_y + 1, start_h - 2, COLOR_BLACK);
            vga_hline(clock_x, start_y + start_h - 1, 56, COLOR_WHITE);
            vga_vline(clock_x + 55, start_y, start_h, COLOR_WHITE);
            vga_putstring(clock_x + 8, start_y + 7, "12:00", COLOR_BLACK, GUI_COLOR_TASKBAR);
        }
    }
    
    /* Redraw start menu if open and intersects */
    if (start_menu_open) {
        int taskbar_y = SCREEN_HEIGHT - 28;
        int menu_x = 2;
        int menu_y = taskbar_y - 120;
        int menu_w = 140;
        int menu_h = 120;
        
        if (menu_x < left + width && menu_x + menu_w > left &&
            menu_y < top + height && menu_y + menu_h > top) {
            
            vga_fillrect(menu_x, menu_y, menu_w, menu_h, COLOR_LIGHT_GRAY);
            vga_rect(menu_x, menu_y, menu_w, menu_h, COLOR_BLACK);
            
            const char* menu_items[] = {"Programs", "Files", "Settings", "Shutdown", 0};
            int item_h = 20;
            for (int j = 0; menu_items[j]; j++) {
                int item_y = menu_y + j * item_h;
                vga_putstring(menu_x + 8, item_y + 6, menu_items[j], COLOR_BLACK, COLOR_LIGHT_GRAY);
            }
        }
    }
    
    /* Redraw windows that intersect this area */
    for (int i = 0; i < num_windows; i++) {
        gui_window_t* win = &windows[i];
        if (!win->visible) continue;
        
        if (win->x < left + width && win->x + win->width > left &&
            win->y < top + height && win->y + win->height > top) {
            
            /* Redraw this window */
            gui_draw_window(win);
            
            /* Redraw its buttons */
            for (int j = 0; j < num_buttons; j++) {
                gui_button_t* btn = &buttons[j];
                if (btn->window_id == i && btn->visible) {
                    gui_draw_button(btn);
                }
            }
        }
    }
    
    /* Redraw app contents for windows in this area */
    for (int i = 0; i < num_windows; i++) {
        gui_window_t* win = &windows[i];
        if (!win->visible) continue;
        
        if (win->x < left + width && win->x + win->width > left &&
            win->y < top + height && win->y + win->height > top) {
            
            /* Redraw app content */
            if (i == get_browser_win()) browser_draw_content(win);
            else if (i == get_files_win()) files_draw_content(win);
            else if (i == get_notepad_win()) notepad_draw_content(win);
            else if (i == get_terminal_win()) terminal_draw_content(win);
            else if (i == get_calc_win()) calc_draw_content(win);
            else if (i == get_settings_win()) settings_draw_content(win);
            else if (i == get_about_win()) about_draw_content(win);
        }
    }
/* Draw cursor at position - called AFTER all other rendering */
static void draw_cursor_at(int x, int y) {
    for (int j = 0; j < 16; j++) {
        for (int i = 0; i < 12; i++) {
            int px = x + i;
            int py = y + j;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                uint8_t val = cursor_shape[j][i];
                if (val == 1) {
                    vga_putpixel(px, py, COLOR_BLACK);
                } else if (val == 2) {
                    vga_putpixel(px, py, COLOR_WHITE);
                }
            }
        }
    }
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
    cursor_visible = 0;
    cursor_x = SCREEN_WIDTH / 2;
    cursor_y = SCREEN_HEIGHT / 2;
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

/* Get active window */
int gui_get_active_window(void) {
    return active_window;
}

/* Close window (hide it) */
void gui_close_window(int window_id) {
    if (window_id >= 0 && window_id < num_windows) {
        windows[window_id].visible = 0;
        windows[window_id].active = 0;
        if (active_window == window_id) {
            active_window = -1;
        }
    }
}

/* ============================================================================
 * CURSOR API 2.0 - Optimized rectangle redraws
 * ============================================================================ */

/* Draw cursor - optimized: only redraws cursor area when moved */
void gui_draw_cursor(int x, int y) {
    /* Clamp position */
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > SCREEN_WIDTH - CURSOR_WIDTH) x = SCREEN_WIDTH - CURSOR_WIDTH;
    if (y > SCREEN_HEIGHT - CURSOR_HEIGHT) y = SCREEN_HEIGHT - CURSOR_HEIGHT;
    
    /* If cursor moved, erase old position by redrawing that area */
    if (cursor_last_x >= 0 && cursor_last_y >= 0 && 
        (cursor_last_x != x || cursor_last_y != y)) {
        /* Call kernel function to redraw cursor area */
        extern void redraw_cursor_area_kernel(int x, int y);
        redraw_cursor_area_kernel(cursor_last_x, cursor_last_y);
    }
    
    /* Update position */
    cursor_x = x;
    cursor_y = y;
    cursor_visible = 1;
    
    /* Draw cursor at new position */
    draw_cursor_at(x, y);
    
    /* Update last position */
    cursor_last_x = x;
    cursor_last_y = y;
}

/* Erase cursor - in 2.0, this is a no-op since we redraw everything */
void gui_erase_cursor(void) {
    cursor_visible = 0;
}

/* Invalidate cursor - in 2.0, just marks cursor as not drawn */
void gui_cursor_invalidate(void) {
    cursor_visible = 0;
}

/* ============================================================================
 * DIRTY RECT STUBS (kept for API compatibility)
 * ============================================================================ */

static int num_dirty_rects = 0;

void gui_add_dirty_rect(int x, int y, int width, int height) {
    (void)x; (void)y; (void)width; (void)height;
}

int gui_has_dirty_rects(void) {
    return num_dirty_rects > 0;
}

void gui_clear_dirty_rects(void) {
    num_dirty_rects = 0;
}

void gui_redraw_dirty(void) {
    num_dirty_rects = 0;
}

/* ============================================================================
 * DRAWING FUNCTIONS
 * ============================================================================ */

/* Draw desktop */
void gui_draw_desktop(void) {
    vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - 28, GUI_COLOR_DESKTOP);
}

/* Draw menubar/taskbar */
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
    
    /* Windows logo (simplified colored squares) */
    vga_fillrect(start_x + 5, start_y + 5, 5, 5, COLOR_RED);
    vga_fillrect(start_x + 5, start_y + 11, 5, 5, COLOR_BLUE);
    vga_fillrect(start_x + 11, start_y + 5, 5, 5, COLOR_GREEN);
    vga_fillrect(start_x + 11, start_y + 11, 5, 5, COLOR_YELLOW);
    
    /* Start text */
    vga_putstring(start_x + 20, start_y + 7, "Start", COLOR_BLACK, GUI_COLOR_BUTTON_BG);
    
    /* Clock area (sunken) */
    int clock_x = SCREEN_WIDTH - 60;
    vga_fillrect(clock_x, start_y, 56, start_h, GUI_COLOR_TASKBAR);
    vga_hline(clock_x, start_y, 56, COLOR_DARK_GRAY);
    vga_vline(clock_x, start_y, start_h, COLOR_DARK_GRAY);
    vga_hline(clock_x + 1, start_y + 1, 54, COLOR_BLACK);
    vga_vline(clock_x + 1, start_y + 1, start_h - 2, COLOR_BLACK);
    vga_hline(clock_x, start_y + start_h - 1, 56, COLOR_WHITE);
    vga_vline(clock_x + 55, start_y, start_h, COLOR_WHITE);
    vga_putstring(clock_x + 8, start_y + 7, "12:00", COLOR_BLACK, GUI_COLOR_TASKBAR);
}

/* Draw window */
void gui_draw_window(gui_window_t* win) {
    if (!win->visible) return;
    
    int x = win->x;
    int y = win->y;
    int w = win->width;
    int h = win->height;
    
    /* Window background */
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
    
    /* Close button (X) */
    int btn_width = 16;
    int btn_height = 14;
    int btn_y = y + 5;
    int close_x = x + w - btn_width - 6;
    
    /* Red background for close button */
    vga_fillrect(close_x, btn_y, btn_width, btn_height, COLOR_RED);
    vga_hline(close_x, btn_y, btn_width, COLOR_LIGHT_RED);
    vga_vline(close_x, btn_y, btn_height, COLOR_LIGHT_RED);
    vga_hline(close_x, btn_y + btn_height - 1, btn_width, COLOR_BROWN);
    vga_vline(close_x + btn_width - 1, btn_y, btn_height, COLOR_BROWN);
    
    /* Draw X */
    int cx = close_x + btn_width / 2;
    int cy = btn_y + btn_height / 2;
    for (int d = -3; d <= 3; d++) {
        vga_putpixel(cx + d, cy + d, COLOR_WHITE);
        vga_putpixel(cx + d, cy - d, COLOR_WHITE);
        vga_putpixel(cx + d + 1, cy + d, COLOR_WHITE);
        vga_putpixel(cx + d + 1, cy - d, COLOR_WHITE);
    }
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
        y += win->y + 16;
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
    
    /* Button 3D effect */
    if (!btn->pressed) {
        vga_hline(x + 1, y + 1, btn->width - 2, COLOR_WHITE);
        vga_vline(x + 1, y + 1, btn->height - 2, COLOR_WHITE);
        vga_hline(x + 1, y + btn->height - 2, btn->width - 2, COLOR_DARK_GRAY);
        vga_vline(x + btn->width - 2, y + 1, btn->height - 2, COLOR_DARK_GRAY);
    } else {
        vga_hline(x + 1, y + 1, btn->width - 2, COLOR_DARK_GRAY);
        vga_vline(x + 1, y + 1, btn->height - 2, COLOR_DARK_GRAY);
    }
    
    /* Text (centered) */
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

/* Update GUI - handle input */
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
                if (win->y + win->height > SCREEN_HEIGHT - 28)
                    win->y = SCREEN_HEIGHT - 28 - win->height;
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
        int close_x = win->x + win->width - 22;
        int close_y = win->y + 5;
        if (clicked && point_in_rect(mx, my, close_x, close_y, 16, 14)) {
            win->visible = 0;
            return;
        }
        
        /* Check title bar for dragging */
        if (clicked && point_in_rect(mx, my, win->x, win->y, win->width, 20)) {
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

/* Draw entire GUI (windows and buttons only) */
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
