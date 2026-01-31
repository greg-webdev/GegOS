/*
 * kernel.c - GegOS Kernel v0.3 (Optimized)
 * Stable graphical kernel with reduced flicker
 */

#include <stdint.h>
#include <stddef.h>
#include "io.h"
#include "vga.h"
#include "keyboard.h"
#include "mouse.h"
#include "gui.h"
#include "apps.h"

/* External app window getters */
extern int get_browser_win(void);
extern int get_files_win(void);
extern int get_notepad_win(void);
extern int get_terminal_win(void);
extern int get_calc_win(void);
extern int get_about_win(void);
extern int get_settings_win(void);

/* External app content drawers */
extern void browser_draw_content(gui_window_t* win);
extern void files_draw_content(gui_window_t* win);
extern void notepad_draw_content(gui_window_t* win);
extern void terminal_draw_content(gui_window_t* win);
extern void calc_draw_content(gui_window_t* win);
extern void about_draw_content(gui_window_t* win);
extern void settings_draw_content(gui_window_t* win);

/* External app input handlers */
extern void browser_handle_key(char key);
extern void files_handle_key(char key);
extern void files_handle_click(gui_window_t* win, int mx, int my);
extern void notepad_handle_key(char key);
extern void terminal_handle_key(char key);
extern void calc_handle_key(char key);
extern void calc_handle_click(gui_window_t* win, int mx, int my);
extern void settings_handle_click(gui_window_t* win, int mx, int my);

/* Settings getters */
extern int get_settings_theme(void);
extern int get_settings_mouse_speed(void);

/* Global redraw flag */
static int needs_redraw = 1;

/* Desktop icon positions */
typedef struct {
    int x, y;
    const char* label;
    void (*action)(void);
} desktop_icon_t;

/* Icon click handlers */
static void click_browser(void) { app_browser(); needs_redraw = 1; }
static void click_files(void) { app_files(); needs_redraw = 1; }
static void click_notepad(void) { app_notepad(); needs_redraw = 1; }
static void click_terminal(void) { app_terminal(); needs_redraw = 1; }
static void click_calc(void) { app_calculator(); needs_redraw = 1; }
static void click_settings(void) { app_settings(); needs_redraw = 1; }
static void click_about(void) { app_about(); needs_redraw = 1; }

static desktop_icon_t desktop_icons[] = {
    {10, 25, "Browser", click_browser},
    {10, 70, "Files", click_files},
    {10, 115, "Notepad", click_notepad},
    {10, 160, "Terminal", click_terminal},
    {260, 25, "Calc", click_calc},
    {260, 70, "Settings", click_settings},
    {260, 115, "About", click_about},
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

/* Draw desktop icons */
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

/* Check icon clicks */
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
static void handle_app_keyboard(char key) {
    gui_window_t* win;
    
    win = gui_get_window(get_browser_win());
    if (win && win->visible && win->active) {
        browser_handle_key(key);
        needs_redraw = 1;
        return;
    }
    
    win = gui_get_window(get_files_win());
    if (win && win->visible && win->active) {
        files_handle_key(key);
        needs_redraw = 1;
        return;
    }
    
    win = gui_get_window(get_notepad_win());
    if (win && win->visible && win->active) {
        notepad_handle_key(key);
        needs_redraw = 1;
        return;
    }
    
    win = gui_get_window(get_terminal_win());
    if (win && win->visible && win->active) {
        terminal_handle_key(key);
        needs_redraw = 1;
        return;
    }
    
    win = gui_get_window(get_calc_win());
    if (win && win->visible && win->active) {
        calc_handle_key(key);
        needs_redraw = 1;
        return;
    }
}

/* Handle mouse click for active app */
static void handle_app_click(int mx, int my) {
    gui_window_t* win;
    
    win = gui_get_window(get_files_win());
    if (win && win->visible && win->active) {
        if (mx >= win->x && mx < win->x + win->width &&
            my >= win->y + 16 && my < win->y + win->height) {
            files_handle_click(win, mx, my);
            needs_redraw = 1;
            return;
        }
    }
    
    win = gui_get_window(get_calc_win());
    if (win && win->visible && win->active) {
        if (mx >= win->x && mx < win->x + win->width &&
            my >= win->y + 16 && my < win->y + win->height) {
            calc_handle_click(win, mx, my);
            needs_redraw = 1;
            return;
        }
    }
    
    win = gui_get_window(get_settings_win());
    if (win && win->visible && win->active) {
        if (mx >= win->x && mx < win->x + win->width &&
            my >= win->y + 16 && my < win->y + win->height) {
            settings_handle_click(win, mx, my);
            needs_redraw = 1;
            return;
        }
    }
}

/* Draw everything once */
static void full_redraw(void) {
    /* Wait for vsync before drawing to reduce tearing */
    vga_vsync();
    
    /* Draw desktop with current theme color */
    vga_fillrect(0, 13, SCREEN_WIDTH, SCREEN_HEIGHT - 13, get_desktop_color());
    
    /* Draw menubar */
    gui_draw_menubar();
    
    /* Draw desktop icons */
    draw_desktop_icons();
    
    /* Draw windows (from gui.c but skip cursor) */
    gui_draw();
    
    /* Draw app contents inside windows */
    draw_app_contents();
}

/* Kernel main entry point */
void kernel_main(uint32_t magic, uint32_t* multiboot_info) {
    (void)magic;
    (void)multiboot_info;
    
    /* Initialize VGA graphics mode */
    vga_init();
    
    /* Show loading screen */
    vga_clear(COLOR_BLUE);
    vga_fillrect(80, 70, 160, 60, COLOR_WHITE);
    vga_rect(80, 70, 160, 60, COLOR_BLACK);
    vga_putstring(110, 85, "GegOS v0.3", COLOR_BLACK, COLOR_WHITE);
    vga_putstring(95, 105, "Starting...", COLOR_DARK_GRAY, COLOR_WHITE);
    
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
    
    /* Clear mouse buffer */
    for (int i = 0; i < 50; i++) {
        mouse_update();
    }
    
    /* Initialize GUI and apps */
    gui_init();
    apps_init();
    
    /* Initial draw */
    needs_redraw = 1;
    
    /* Previous mouse state */
    int last_mx = -1, last_my = -1;
    int last_mouse_btn = 0;
    
    /* Main loop */
    while (1) {
        /* === INPUT HANDLING === */
        
        /* Update mouse state */
        mouse_update();
        
        int mx = mouse_get_x();
        int my = mouse_get_y();
        int mouse_btn = mouse_button_down(MOUSE_LEFT);
        int mouse_clicked = mouse_btn && !last_mouse_btn;
        int mouse_moved = (mx != last_mx || my != last_my);
        
        /* Track dragging for windows */
        static int was_dragging = 0;
        
        /* Handle mouse clicks */
        if (mouse_clicked) {
            gui_update();
            
            /* Check desktop icons */
            if (my > 12) {
                check_icon_click(mx, my);
            }
            
            /* Handle app-specific clicks */
            handle_app_click(mx, my);
            
            needs_redraw = 1;
        } else if (mouse_btn) {
            /* Dragging */
            gui_update();
            was_dragging = 1;
            needs_redraw = 1;
        } else if (was_dragging) {
            was_dragging = 0;
            needs_redraw = 1;
        }
        
        last_mouse_btn = mouse_btn;
        
        /* Handle keyboard input */
        if (keyboard_haskey()) {
            char key = keyboard_getchar();
            if (key != 0) {
                handle_app_keyboard(key);
            }
        }
        
        /* === RENDERING === */
        
        /* Only redraw when something changed */
        if (needs_redraw) {
            full_redraw();
            needs_redraw = 0;
            last_mx = -1;  /* Force cursor redraw */
        }
        
        /* Always update cursor position smoothly */
        if (mouse_moved || last_mx == -1) {
            gui_draw_cursor(mx, my);
            last_mx = mx;
            last_my = my;
        }
        
        /* Small delay to prevent CPU hogging */
        for (volatile int i = 0; i < 5000; i++);
    }
}
