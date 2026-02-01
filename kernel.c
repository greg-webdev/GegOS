/*
 * kernel.c - GegOS Kernel v0.4 (Optimized Rendering)
 * Only redraws areas that changed - no more full screen flicker
 */

#include <stdint.h>
#include <stddef.h>
#include "io.h"
#include "vga.h"
#include "keyboard.h"
#include "mouse.h"
#include "gui.h"
#include "apps.h"
#include "network.h"
#include "wifi.h"

/* Multiboot 2 structures for framebuffer support */
typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} multiboot2_info_header_t;

typedef struct {
    uint32_t type;
    uint32_t size;
} multiboot2_tag_header_t;

typedef struct {
    uint32_t type;        // 8
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} multiboot2_framebuffer_tag_t;

/* Global framebuffer information - unused in 32-bit kernel */
/*
static uint64_t fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t fb_bpp = 0;
*/

/* Parse Multiboot 2 information structure - commented out for 32-bit kernel */
/*
static void parse_multiboot2_info(uint32_t* mb_info) {
    multiboot2_info_header_t* header = (multiboot2_info_header_t*)mb_info;
    uint32_t total_size = header->total_size;
    
    uint32_t offset = 8; // Skip header
    while (offset < total_size) {
        multiboot2_tag_header_t* tag = (multiboot2_tag_header_t*)((uint8_t*)mb_info + offset);
        
        if (tag->type == 8) { // Framebuffer tag
            multiboot2_framebuffer_tag_t* fb_tag = (multiboot2_framebuffer_tag_t*)tag;
            fb_addr = fb_tag->framebuffer_addr;
            fb_pitch = fb_tag->framebuffer_pitch;
            fb_width = fb_tag->framebuffer_width;
            fb_height = fb_tag->framebuffer_height;
            fb_bpp = fb_tag->framebuffer_bpp;
        } else if (tag->type == 0) { // End tag
            break;
        }
        
        offset += (tag->size + 7) & ~7;
    }
}
*/

/* External app window getters */
extern int get_browser_win(void);
extern int get_wifi_win(void);
extern int get_files_win(void);
extern int get_notepad_win(void);
extern int get_terminal_win(void);
extern int get_calc_win(void);
extern int get_about_win(void);
extern int get_settings_win(void);

/* External app content drawers */
extern void browser_draw_content(gui_window_t* win);
extern void wifi_draw_content(gui_window_t* win);
extern void files_draw_content(gui_window_t* win);
extern void notepad_draw_content(gui_window_t* win);
extern void terminal_draw_content(gui_window_t* win);
extern void calc_draw_content(gui_window_t* win);
extern void about_draw_content(gui_window_t* win);
extern void settings_draw_content(gui_window_t* win);

/* External app input handlers */
extern void browser_handle_key(char key);
extern void wifi_handle_key(char key);
extern void wifi_handle_click(int x, int y);
extern void files_handle_key(char key);
extern void files_handle_click(gui_window_t* win, int mx, int my);
extern void notepad_handle_key(char key);
extern void terminal_key_handler(char key);
extern void calc_handle_key(char key);
extern void calc_handle_click(gui_window_t* win, int mx, int my);
extern void settings_handle_click(gui_window_t* win, int mx, int my);

/* Game functions */
extern void pong_run(void);
extern void snake_run(void);
extern void game_2048_run(void);

/* Settings getters */
extern int get_settings_theme(void);
extern int get_settings_mouse_speed(void);

/* Global redraw flag */
static int needs_redraw = 1;

/* Taskbar height constant */
#define TASKBAR_HEIGHT 32

/* Start menu state */
static int start_menu_open = 0;

/* Desktop icon positions */

/* Icon click handlers */
static void click_wifi(void) { app_wifi(); needs_redraw = 1; }
static void click_browser(void) { app_browser(); needs_redraw = 1; }
static void click_files(void) { app_files(); needs_redraw = 1; }
static void click_notepad(void) { app_notepad(); needs_redraw = 1; }
static void click_terminal(void) { app_terminal(); needs_redraw = 1; }
static void click_calc(void) { app_calculator(); needs_redraw = 1; }
static void click_settings(void) { app_settings(); needs_redraw = 1; }
static void click_about(void) { app_about(); needs_redraw = 1; }

static desktop_icon_t desktop_icons[] = {
    {20, 40, "Potato", click_browser},
    {20, 80, "WiFi", click_wifi},
    {20, 100, "Files", click_files},
    {20, 160, "Notepad", click_notepad},
    {20, 220, "Terminal", click_terminal},
    {20, TASKBAR_HEIGHT, "Calc", click_calc},
    {20, 340, "Settings", click_settings},
    {20, 400, "About", click_about},
    {0, 0, 0, 0}
};

/* Get theme color */
static uint8_t get_desktop_color(void) {
    int theme = get_settings_theme();
    switch (theme) {
        case 0: return COLOR_CYAN;
        case 1: return COLOR_LIGHT_GRAY;
        case 2: return COLOR_BLUE;
        default: return COLOR_CYAN;
    }
}

/* Forward declarations */
static void redraw_start_menu_area(void);

/* Draw desktop icons */
static void draw_desktop_icons(void) __attribute__((unused));
static void draw_desktop_icons(void) {
    for (int i = 0; desktop_icons[i].label; i++) {
        int x = desktop_icons[i].x;
        int y = desktop_icons[i].y;
        
        /* Icon box */
        vga_fillrect(x, y, 48, 32, COLOR_WHITE);
        vga_rect(x, y, 48, 32, COLOR_BLACK);
        /* Icon symbol */
        vga_fillrect(x + 14, y + 4, 20, 16, COLOR_BLUE);
        
        /* Label */
        int label_len = 0;
        const char* s = desktop_icons[i].label;
        while (*s++) label_len++;
        int lx = x + (48 - label_len * 8) / 2;
        
        vga_putstring(lx, y + 23, desktop_icons[i].label, COLOR_BLACK, COLOR_WHITE);
    }
}

/* Handle start menu button click and menu item clicks */
static int handle_start_menu_click(int mx, int my) {
    int taskbar_y = SCREEN_HEIGHT - TASKBAR_HEIGHT;
    int start_x = 2;
    int start_y = taskbar_y + 3;
    int start_w = 60;
    int start_h = 22;
    
    /* Check if start button was clicked */
    if (mx >= start_x && mx < start_x + start_w &&
        my >= start_y && my < start_y + start_h) {
        /* Toggle start menu */
        start_menu_open = !start_menu_open;
        redraw_start_menu_area();  /* Only redraw menu area */
        return 1;
    }
    
    /* If start menu is open, check menu items */
    if (start_menu_open) {
        /* Start menu items are at: x=2, starting at y=taskbar_y-120 */
        int menu_x = start_x;
        int menu_y = taskbar_y - 120;
        int menu_w = 140;
        int item_h = 20;
        
        /* Check for menu item clicks */
        if (mx >= menu_x && mx < menu_x + menu_w && my >= menu_y) {
            int item = (my - menu_y) / item_h;
            
            /* Menu items: Programs (0), Files (1), Settings (2), Shutdown (3) */
            if (item == 0) {
                /* Programs - open file browser for now */
                get_files_win();
                start_menu_open = 0;
                needs_redraw = 1;
                return 1;
            } else if (item == 1) {
                /* Files */
                gui_window_t* win = gui_get_window(get_files_win());
                if (win) {
                    win->visible = 1;
                    win->active = 1;
                }
                start_menu_open = 0;
                needs_redraw = 1;
                return 1;
            } else if (item == 2) {
                /* Settings */
                gui_window_t* win = gui_get_window(get_settings_win());
                if (win) {
                    win->visible = 1;
                    win->active = 1;
                }
                start_menu_open = 0;
                needs_redraw = 1;
                return 1;
            } else if (item == 3) {
                /* Shutdown - just hide all windows and show message */
                start_menu_open = 0;
                needs_redraw = 1;
                return 1;
            }
        }
        /* Click outside menu closes it */
        start_menu_open = 0;
        redraw_start_menu_area();  /* Only redraw menu area */
        return 1;
    }
    
    return 0;
}

static int check_icon_click(int mx, int my) {
    for (int i = 0; desktop_icons[i].label; i++) {
        int x = desktop_icons[i].x;
        int y = desktop_icons[i].y;
        
        if (mx >= x && mx < x + 48 && my >= y && my < y + 32) {
            if (desktop_icons[i].action) {
                desktop_icons[i].action();
                return 1;
            }
        }
    }
    return 0;
}

/* Draw app content based on window */
static void draw_app_contents(void) {
    gui_window_t* win;
    
    win = gui_get_window(get_browser_win());
    if (win && win->visible) browser_draw_content(win);
    
    win = gui_get_window(get_wifi_win());
    if (win && win->visible) wifi_draw_content(win);
    
    win = gui_get_window(get_files_win());
    if (win && win->visible) files_draw_content(win);
    
    win = gui_get_window(get_notepad_win());
    if (win && win->visible) notepad_draw_content(win);
    
    win = gui_get_window(get_terminal_win());
    if (win && win->visible) terminal_draw_content(win);
    
    win = gui_get_window(get_calc_win());
    if (win && win->visible) calc_draw_content(win);
    
    win = gui_get_window(get_settings_win());
    if (win && win->visible) settings_draw_content(win);
    
    win = gui_get_window(get_about_win());
    if (win && win->visible) about_draw_content(win);
}

/* Handle keyboard for active app */
static void handle_app_keyboard(char key, int mx, int my) {
    gui_window_t* win;
    
    /* Erase cursor before any drawing */
    gui_erase_cursor();
    
    win = gui_get_window(get_wifi_win());
    if (win && win->visible && win->active) {
        wifi_handle_key(key);
        wifi_draw_content(win);
        gui_cursor_invalidate();
        gui_draw_cursor(mx, my);
        return;
    }
    
    win = gui_get_window(get_browser_win());
    if (win && win->visible && win->active) {
        browser_handle_key(key);
        browser_draw_content(win);
        gui_cursor_invalidate();
        gui_draw_cursor(mx, my);
        return;
    }
    
    win = gui_get_window(get_files_win());
    if (win && win->visible && win->active) {
        files_handle_key(key);
        files_draw_content(win);
        gui_cursor_invalidate();
        gui_draw_cursor(mx, my);
        return;
    }
    
    win = gui_get_window(get_notepad_win());
    if (win && win->visible && win->active) {
        notepad_handle_key(key);
        notepad_draw_content(win);
        gui_cursor_invalidate();
        gui_draw_cursor(mx, my);
        return;
    }
    
    win = gui_get_window(get_terminal_win());
    if (win && win->visible && win->active) {
        terminal_key_handler(key);
        terminal_draw_content(win);
        gui_cursor_invalidate();
        gui_draw_cursor(mx, my);
        return;
    }
    
    win = gui_get_window(get_calc_win());
    if (win && win->visible && win->active) {
        calc_handle_key(key);
        calc_draw_content(win);
        gui_cursor_invalidate();
        gui_draw_cursor(mx, my);
        return;
    }
}

/* Handle mouse click for active app */
static int handle_app_click(int mx, int my) {
    gui_window_t* win;
    
    /* Erase cursor before any drawing */
    gui_erase_cursor();
    
    win = gui_get_window(get_files_win());
    if (win && win->visible && win->active) {
        if (mx >= win->x && mx < win->x + win->width &&
            my >= win->y + 16 && my < win->y + win->height) {
            files_handle_click(win, mx, my);
            files_draw_content(win);
            gui_cursor_invalidate();
            gui_draw_cursor(mx, my);
            return 1;
        }
    }
    
    win = gui_get_window(get_calc_win());
    if (win && win->visible && win->active) {
        if (mx >= win->x && mx < win->x + win->width &&
            my >= win->y + 16 && my < win->y + win->height) {
            calc_handle_click(win, mx, my);
            calc_draw_content(win);
            gui_cursor_invalidate();
            gui_draw_cursor(mx, my);
            return 1;
        }
    }
    
    win = gui_get_window(get_settings_win());
    if (win && win->visible && win->active) {
        if (mx >= win->x && mx < win->x + win->width &&
            my >= win->y + 16 && my < win->y + win->height) {
            settings_handle_click(win, mx, my);
            settings_draw_content(win);
            gui_cursor_invalidate();
            gui_draw_cursor(mx, my);
            return 1;
        }
    }
    return 0;
}

/* Draw everything - optimized partial redraws */
static void full_redraw(void) __attribute__((unused));
static void full_redraw(void) {
    /* Erase cursor properly before redrawing */
    gui_erase_cursor();

    /* Wait for vsync before drawing to reduce tearing */
    vga_vsync();

    /* Only redraw desktop if needed - for now we redraw everything */
    /* TODO: Implement dirty rectangles for partial redraws */
    vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - TASKBAR_HEIGHT, get_desktop_color());

    /* Draw desktop icons */
    for (int i = 0; desktop_icons[i].label; i++) {
        int ix = desktop_icons[i].x;
        int iy = desktop_icons[i].y;

        /* Icon box */
        vga_fillrect(ix, iy, 48, 32, COLOR_WHITE);
        vga_rect(ix, iy, 48, 32, COLOR_BLACK);
        /* Icon symbol */
        vga_fillrect(ix + 14, iy + 4, 20, 16, COLOR_BLUE);

        /* Label */
        int label_len = 0;
        const char* s = desktop_icons[i].label;
        while (*s++) label_len++;
        int lx = ix + (48 - label_len * 8) / 2;
        vga_putstring(lx, iy + 23, desktop_icons[i].label, COLOR_BLACK, COLOR_WHITE);
    }

    /* Draw taskbar */
    gui_draw_menubar();

    /* Draw start menu if open */
    if (start_menu_open) {
        int taskbar_y = SCREEN_HEIGHT - TASKBAR_HEIGHT;
        int menu_x = 2;
        int menu_y = taskbar_y - 120;
        int menu_w = 140;
        int menu_h = 120;
        int item_h = 20;

        /* Menu background with border */
        vga_fillrect(menu_x, menu_y, menu_w, menu_h, COLOR_LIGHT_GRAY);
        vga_rect(menu_x, menu_y, menu_w, menu_h, COLOR_BLACK);

        /* Menu items: Programs, Files, Settings, Shutdown */
        const char* menu_items[] = {"Programs", "Files", "Settings", "Shutdown", 0};
        for (int i = 0; menu_items[i]; i++) {
            int item_y = menu_y + i * item_h;
            vga_putstring(menu_x + 8, item_y + 6, menu_items[i], COLOR_BLACK, COLOR_LIGHT_GRAY);
            /* Separator line */
            if (i == 2) {
                vga_hline(menu_x + 2, item_y + item_h - 2, menu_w - 4, COLOR_LIGHT_GRAY);
            }
        }
    }

    /* Draw windows */
    gui_draw();

    /* Draw app contents inside windows */
    draw_app_contents();

    /* Invalidate cursor backup since screen changed */
    gui_cursor_invalidate();
}

/* Redraw only the start menu area */
static void redraw_start_menu_area(void) {
    gui_erase_cursor();
    
    int taskbar_y = SCREEN_HEIGHT - TASKBAR_HEIGHT;
    int menu_x = 2;
    int menu_y = taskbar_y - 120;
    int menu_w = 140;
    int menu_h = 120;
    
    if (start_menu_open) {
        /* Draw menu */
        int item_h = 20;
        vga_fillrect(menu_x, menu_y, menu_w, menu_h, COLOR_LIGHT_GRAY);
        vga_rect(menu_x, menu_y, menu_w, menu_h, COLOR_BLACK);
        
        const char* menu_items[] = {"Programs", "Files", "Settings", "Shutdown", 0};
        for (int i = 0; menu_items[i]; i++) {
            int item_y = menu_y + i * item_h;
            vga_putstring(menu_x + 8, item_y + 6, menu_items[i], COLOR_BLACK, COLOR_LIGHT_GRAY);
        }
    } else {
        /* Erase menu area - redraw desktop portion */
        vga_fillrect(menu_x, menu_y, menu_w, menu_h, get_desktop_color());
        /* Redraw any icons that might be there */
        for (int i = 0; desktop_icons[i].label; i++) {
            int ix = desktop_icons[i].x;
            int iy = desktop_icons[i].y;
            /* Check if icon overlaps menu area */
            if (ix < menu_x + menu_w && ix + 48 > menu_x &&
                iy < menu_y + menu_h && iy + 40 > menu_y) {
                vga_fillrect(ix, iy, 48, 32, COLOR_WHITE);
                vga_rect(ix, iy, 48, 32, COLOR_BLACK);
                vga_fillrect(ix + 14, iy + 4, 20, 16, COLOR_BLUE);
                int label_len = 0;
                const char* s = desktop_icons[i].label;
                while (*s++) label_len++;
                int lx = ix + (48 - label_len * 8) / 2;
                vga_putstring(lx, iy + 23, desktop_icons[i].label, COLOR_BLACK, COLOR_WHITE);
            }
        }
    }
    
    gui_cursor_invalidate();
}

/* Redraw the area around a cursor position (30x30 pixel area) */
void redraw_cursor_area_kernel(int x, int y) {
    /* Calculate redraw rectangle: 30x30 area starting at cursor position */
    int left = x;
    int top = y;
    int width = 30;
    int height = 30;
    
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
    int taskbar_y = SCREEN_HEIGHT - TASKBAR_HEIGHT;
    if (top + height > taskbar_y) {
        /* Redraw taskbar in the intersecting area */
        int tb_left = left;
        int tb_width = width;
        if (tb_left < 0) tb_left = 0;
        if (tb_left + tb_width > SCREEN_WIDTH) tb_width = SCREEN_WIDTH - tb_left;
        
        vga_fillrect(tb_left, taskbar_y, tb_width, TASKBAR_HEIGHT, COLOR_LIGHT_GRAY);
        
        /* Redraw taskbar elements if they intersect */
        if (tb_left < 60) {  /* Start button */
            int start_w = 60;
            int start_h = 22;
            int start_x = 2;
            int start_y = taskbar_y + 3;
            
            vga_fillrect(start_x, start_y, start_w, start_h, COLOR_LIGHT_GRAY);
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
            
            vga_putstring(start_x + 20, start_y + 7, "Start", COLOR_BLACK, COLOR_LIGHT_GRAY);
        }
        
        if (tb_left + tb_width > SCREEN_WIDTH - 60) {  /* Clock */
            int clock_x = SCREEN_WIDTH - 60;
            int start_y = taskbar_y + 3;
            int start_h = 22;
            
            vga_fillrect(clock_x, start_y, 56, start_h, COLOR_LIGHT_GRAY);
            vga_hline(clock_x, start_y, 56, COLOR_DARK_GRAY);
            vga_vline(clock_x, start_y, start_h, COLOR_DARK_GRAY);
            vga_hline(clock_x + 1, start_y + 1, 54, COLOR_BLACK);
            vga_vline(clock_x + 1, start_y + 1, start_h - 2, COLOR_BLACK);
            vga_hline(clock_x, start_y + start_h - 1, 56, COLOR_WHITE);
            vga_vline(clock_x + 55, start_y, start_h, COLOR_WHITE);
            vga_putstring(clock_x + 8, start_y + 7, "12:00", COLOR_BLACK, COLOR_LIGHT_GRAY);
        }
    }
    
    /* Redraw start menu if open and intersects */
    if (start_menu_open) {
        int taskbar_y = SCREEN_HEIGHT - TASKBAR_HEIGHT;
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
    
    /* Redraw windows that intersect this area - DISABLED to prevent cursor flicker */
    /* Windows should remain drawn when cursor moves over them */
    for (int i = 0; i < 16; i++) {  /* MAX_WINDOWS */
        gui_window_t* win = gui_get_window(i);
        if (!win || !win->visible) continue;
        
        if (win->x < left + width && win->x + win->width > left &&
            win->y < top + height && win->y + win->height > top) {
            
            /* Redraw this window */
            gui_draw_window(win);
            
            /* Redraw its buttons */
            for (int j = 0; j < 32; j++) {  /* MAX_BUTTONS */
                gui_button_t* btn = NULL; // gui_get_button(j); - need to add this function
                if (btn && btn->window_id == i && btn->visible) {
                    gui_draw_button(btn);
                }
            }
        }
    }
    
    /* Redraw app contents for windows in this area */
    for (int i = 0; i < 16; i++) {  /* MAX_WINDOWS */
        gui_window_t* win = gui_get_window(i);
        if (!win || !win->visible) continue;
        
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
}

/* Redraw only a specific window and its content - for future use */
static void redraw_window(int win_id) __attribute__((unused));
static void redraw_window(int win_id) {
    gui_erase_cursor();
    
    gui_window_t* win = gui_get_window(win_id);
    if (!win || !win->visible) return;
    
    /* Draw this window */
    gui_draw_window(win);
    
    /* Draw its content */
    if (win_id == get_browser_win()) browser_draw_content(win);
    else if (win_id == get_files_win()) files_draw_content(win);
    else if (win_id == get_notepad_win()) notepad_draw_content(win);
    else if (win_id == get_terminal_win()) terminal_draw_content(win);
    else if (win_id == get_calc_win()) calc_draw_content(win);
    else if (win_id == get_settings_win()) settings_draw_content(win);
    else if (win_id == get_about_win()) about_draw_content(win);
    
    gui_cursor_invalidate();
}

/* Display games menu and return selected game index, -1 for desktop */
static int show_games_menu(void) {
    /* Game list */
    typedef struct {
        const char* name;
        const char* desc;
    } game_t;
    
    game_t games[] = {
        {"Pong", "Classic Pong game"},
        {"2048", "Tile merging puzzle"},
        {"Snake", "Classic Snake game"},
        {NULL, NULL}
    };
    
    int selected = 0;
    int num_games = 0;
    while (games[num_games].name) num_games++;
    
    /* Draw initial screen ONCE */
    vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_DARK_GRAY);
    vga_putstring(100, 20, "GegOS GAMES", COLOR_YELLOW, COLOR_DARK_GRAY);
    vga_putstring(50, 50, "Select a game (arrow keys, Enter to play, Space for desktop):", 
                 COLOR_WHITE, COLOR_DARK_GRAY);
    vga_putstring(50, SCREEN_HEIGHT - 40, "Up/Down: Move | Enter: Select | Space: Skip to Desktop", 
                 COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
    
    int last_selected = -1;  /* Force initial draw */
    int menu_running = 1;
    
    while (menu_running) {
        /* Only redraw menu items if selection changed */
        if (selected != last_selected) {
            int list_y = 90;
            for (int i = 0; i < num_games; i++) {
                if (i == selected) {
                    vga_fillrect(40, list_y + i * 40, 240, 35, COLOR_BLUE);
                    vga_putstring(50, list_y + i * 40 + 5, games[i].name, COLOR_YELLOW, COLOR_BLUE);
                    vga_putstring(50, list_y + i * 40 + 18, games[i].desc, COLOR_LIGHT_CYAN, COLOR_BLUE);
                } else {
                    vga_fillrect(40, list_y + i * 40, 240, 35, COLOR_DARK_GRAY);
                    vga_rect(40, list_y + i * 40, 240, 35, COLOR_LIGHT_GRAY);
                    vga_putstring(50, list_y + i * 40 + 5, games[i].name, COLOR_WHITE, COLOR_DARK_GRAY);
                    vga_putstring(50, list_y + i * 40 + 18, games[i].desc, COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
                }
            }
            last_selected = selected;
        }
        
        /* Wait for keyboard input */
        while (!keyboard_haskey()) {
            mouse_update();
            if (mouse_button_down(MOUSE_LEFT)) {
                return -1;
            }
        }
        
        char key = keyboard_getchar();
        
        if (key == (char)KEY_UP) {  /* Up arrow */
            selected--;
            if (selected < 0) selected = num_games - 1;
        } else if (key == (char)KEY_DOWN) {  /* Down arrow */
            selected++;
            if (selected >= num_games) selected = 0;
        } else if (key == '\n') {  /* Enter */
            return selected;
        } else if (key == ' ') {  /* Space */
            return -1;
        }
    }
    
    return -1;
}

/* Launch selected game */
static void launch_game(int game_id) {
    switch (game_id) {
        case 0:  /* Pong */
            pong_run();
            break;
        case 1:  /* 2048 */
            game_2048_run();
            break;
        case 2:  /* Snake */
            snake_run();
            break;
    }
}

/* Kernel main entry point */
void kernel_main(uint32_t magic, uint32_t* multiboot_info) {
    (void)magic;
    (void)multiboot_info;
    
    /* Initialize subsystems */
    vga_init();
    keyboard_init();
    mouse_init();
    network_init();
    
    /* Show loading screen */
    vga_clear(COLOR_BLUE);
    vga_fillrect(220, 180, 200, 80, COLOR_WHITE);
    vga_rect(220, 180, 200, 80, COLOR_BLACK);
    vga_putstring(260, 200, "GegOS v1.0", COLOR_BLACK, COLOR_WHITE);
    vga_putstring(250, 230, "Starting...", COLOR_DARK_GRAY, COLOR_WHITE);
    
    /* Delay during startup */
    for (volatile int i = 0; i < 3000000; i++);
    
    /* Initialize input devices */
    keyboard_init();
    
    /* Clear keyboard buffer thoroughly */
    for (volatile int j = 0; j < 500000; j++);
    while (keyboard_haskey()) {
        keyboard_getchar();
    }
    
    mouse_init();
    
    /* Initialize network subsystem */
    network_init();
    
    /* Clear mouse buffer */
    for (int i = 0; i < 50; i++) {
        mouse_update();
    }
    
    /* Initialize GUI and apps */
    gui_init();
    apps_init();
    
    /* Show games menu at startup */
    int selected_game = show_games_menu();
    
    /* If a game was selected, launch it */
    if (selected_game >= 0) {
        launch_game(selected_game);
    }
    
    /* Initial draw */
    needs_redraw = 1;
    
    /* Previous mouse state */
    int last_mx = -1, last_my = -1;
    int last_mouse_btn = 0;
    int active_win_id = -1;
    
    /* Main loop */
    while (1) {
        /* === INPUT HANDLING === */
        
        /* Update mouse state */
        mouse_update();
        
        int mx = mouse_get_x();
        int my = mouse_get_y();
        int mouse_btn = mouse_button_down(MOUSE_LEFT);
        int mouse_clicked = mouse_btn && !last_mouse_btn;
        int mouse_released = !mouse_btn && last_mouse_btn;
        int mouse_moved = (mx != last_mx || my != last_my);
        
        /* Track dragging for windows */
        static int is_dragging = 0;
        
        /* Handle mouse clicks */
        if (mouse_clicked) {
            int old_active = active_win_id;
            gui_update();
            
            /* Check start menu first */
            handle_start_menu_click(mx, my);
            
            /* Check desktop icons */
            if (my > 12) {
                check_icon_click(mx, my);
            }
            
            /* Handle app-specific clicks - returns 1 if handled */
            int app_handled = handle_app_click(mx, my);
            
            /* Find active window */
            active_win_id = -1;
            for (int i = 0; i < 16; i++) {
                gui_window_t* win = gui_get_window(i);
                if (win && win->visible && win->active) {
                    active_win_id = i;
                    break;
                }
            }
            
            /* Only trigger full redraw if window activation changed */
            if (old_active != active_win_id && !app_handled) {
                needs_redraw = 1;
            }
            
            /* Check if we started dragging */
            for (int i = 0; i < 16; i++) {
                gui_window_t* win = gui_get_window(i);
                if (win && win->dragging) {
                    is_dragging = 1;
                    /* Don't redraw while starting drag */
                    break;
                }
            }
        } else if (mouse_btn && is_dragging && mouse_moved) {
            /* Window is being dragged - update position */
            gui_update();
            /* Skip full redraw during drag - too slow */
        } else if (mouse_released) {
            if (is_dragging) {
                is_dragging = 0;
                needs_redraw = 1;  /* Only redraw after drag ends */
            }
        }
        
        last_mouse_btn = mouse_btn;
        
        /* Handle keyboard input */
        if (keyboard_haskey()) {
            char key = keyboard_getchar();
            if (key != 0) {
                /* Alt+F4 closes active window */
                if (key == (char)KEY_F4 && (keyboard_get_modifiers() & MOD_ALT)) {
                    if (active_win_id >= 0) {
                        gui_close_window(active_win_id);
                        active_win_id = -1;
                        needs_redraw = 1;
                    }
                } else {
                    handle_app_keyboard(key, mx, my);
                }
            }
        }
        
        /* === RENDERING === */
        
        /* Redraw screen when needed (major changes only) */
        if (needs_redraw) {
            /* Full redraw - this is the simple, reliable approach */
            vga_vsync();
            
            /* Desktop */
            vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - TASKBAR_HEIGHT, get_desktop_color());
            
            /* Desktop icons */
            for (int i = 0; desktop_icons[i].label; i++) {
                int ix = desktop_icons[i].x;
                int iy = desktop_icons[i].y;
                vga_fillrect(ix, iy, 48, 32, COLOR_WHITE);
                vga_rect(ix, iy, 48, 32, COLOR_BLACK);
                vga_fillrect(ix + 14, iy + 4, 20, 16, COLOR_BLUE);
                int label_len = 0;
                const char* s = desktop_icons[i].label;
                while (*s++) label_len++;
                int lx = ix + (48 - label_len * 8) / 2;
                vga_putstring(lx, iy + 23, desktop_icons[i].label, COLOR_BLACK, COLOR_WHITE);
            }
            
            /* Taskbar */
            gui_draw_menubar();
            
            /* Start menu if open */
            if (start_menu_open) {
                int taskbar_y = SCREEN_HEIGHT - TASKBAR_HEIGHT;
                int menu_x = 2;
                int menu_y = taskbar_y - 120;
                int menu_w = 140;
                int menu_h = 120;
                int item_h = 20;
                vga_fillrect(menu_x, menu_y, menu_w, menu_h, COLOR_LIGHT_GRAY);
                vga_rect(menu_x, menu_y, menu_w, menu_h, COLOR_BLACK);
                const char* menu_items[] = {"Programs", "Files", "Settings", "Shutdown", 0};
                for (int j = 0; menu_items[j]; j++) {
                    int item_y = menu_y + j * item_h;
                    vga_putstring(menu_x + 8, item_y + 6, menu_items[j], COLOR_BLACK, COLOR_LIGHT_GRAY);
                }
            }
            
            /* Windows */
            gui_draw();
            
            /* App contents */
            draw_app_contents();
            
            needs_redraw = 0;
        }
        
        /* Update cursor position - optimized rectangle redraw */
        if (mouse_moved) {
            gui_draw_cursor(mx, my);
        }

        /* Frame rate limiting */
        for (volatile int i = 0; i < 50000; i++);
    }
}
