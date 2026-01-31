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

/* Start menu state */
static int start_menu_open = 0;

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
    {20, 40, "Potato", click_browser},
    {20, 100, "Files", click_files},
    {20, 160, "Notepad", click_notepad},
    {20, 220, "Terminal", click_terminal},
    {20, 280, "Calc", click_calc},
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
    int taskbar_y = SCREEN_HEIGHT - 28;
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

/* Draw everything - called only on major changes */
static void full_redraw(void) {
    /* Erase cursor properly before redrawing */
    gui_erase_cursor();
    
    /* Wait for vsync before drawing to reduce tearing */
    vga_vsync();
    
    /* Draw desktop background */
    vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - 28, get_desktop_color());
    
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
        int taskbar_y = SCREEN_HEIGHT - 28;
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
    
    int taskbar_y = SCREEN_HEIGHT - 28;
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
    
    int menu_running = 1;
    while (menu_running) {
        /* Clear screen with dark background */
        vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_DARK_GRAY);
        
        /* Draw title */
        vga_putstring(100, 20, "GegOS GAMES", COLOR_YELLOW, COLOR_DARK_GRAY);
        vga_putstring(50, 50, "Select a game (arrow keys, Enter to play, Space for desktop):", 
                     COLOR_WHITE, COLOR_DARK_GRAY);
        
        /* Draw game list */
        int list_y = 90;
        for (int i = 0; i < num_games; i++) {
            if (i == selected) {
                /* Selected item - highlight */
                vga_fillrect(40, list_y + i * 40, 240, 35, COLOR_BLUE);
                vga_putstring(50, list_y + i * 40 + 5, games[i].name, COLOR_YELLOW, COLOR_BLUE);
                vga_putstring(50, list_y + i * 40 + 18, games[i].desc, COLOR_LIGHT_CYAN, COLOR_BLUE);
            } else {
                /* Unselected item */
                vga_rect(40, list_y + i * 40, 240, 35, COLOR_LIGHT_GRAY);
                vga_putstring(50, list_y + i * 40 + 5, games[i].name, COLOR_WHITE, COLOR_DARK_GRAY);
                vga_putstring(50, list_y + i * 40 + 18, games[i].desc, COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
            }
        }
        
        /* Draw instructions */
        vga_putstring(50, SCREEN_HEIGHT - 40, "Up/Down: Move | Enter: Select | Space: Skip to Desktop", 
                     COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
        
        /* Wait for keyboard input */
        while (!keyboard_haskey()) {
            mouse_update();
            /* Allow skipping with mouse click */
            if (mouse_button_down(MOUSE_LEFT)) {
                return -1;  /* Go to desktop */
            }
        }
        
        char key = keyboard_getchar();
        
        if (key == 0x48) {  /* Up arrow */
            selected--;
            if (selected < 0) selected = num_games - 1;
        } else if (key == 0x50) {  /* Down arrow */
            selected++;
            if (selected >= num_games) selected = 0;
        } else if (key == '\n') {  /* Enter */
            return selected;
        } else if (key == ' ') {  /* Space */
            return -1;  /* Go to desktop */
        }
    }
    
    return -1;  /* Default to desktop */
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
    
    /* Initialize VGA graphics mode */
    vga_init();
    
    /* Show loading screen */
    vga_clear(COLOR_BLUE);
    vga_fillrect(220, 180, 200, 80, COLOR_WHITE);
    vga_rect(220, 180, 200, 80, COLOR_BLACK);
    vga_putstring(260, 200, "GegOS v0.7", COLOR_BLACK, COLOR_WHITE);
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
                    needs_redraw = 1;  /* Full redraw for drag start */
                    break;
                }
            }
        } else if (mouse_btn && is_dragging && mouse_moved) {
            /* Window is being dragged - update position */
            gui_update();
            needs_redraw = 1;  /* Need redraw for window movement */
        } else if (mouse_released) {
            if (is_dragging) {
                is_dragging = 0;
                needs_redraw = 1;  /* Final redraw after drag */
            }
        }
        
        last_mouse_btn = mouse_btn;
        
        /* Handle keyboard input */
        if (keyboard_haskey()) {
            char key = keyboard_getchar();
            if (key != 0) {
                handle_app_keyboard(key, mx, my);
            }
        }
        
        /* === RENDERING === */
        
        /* Only full redraw when something major changed */
        if (needs_redraw) {
            full_redraw();
            needs_redraw = 0;
            last_mx = -1;  /* Force cursor redraw */
        }
        
        /* Update cursor position - lightweight, save/restore pixels */
        if (mouse_moved || last_mx == -1) {
            gui_draw_cursor(mx, my);
            last_mx = mx;
            last_my = my;
        }
        
        /* Small delay to prevent CPU hogging */
        for (volatile int i = 0; i < 5000; i++);
    }
}
