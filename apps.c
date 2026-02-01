/*
 * apps.c - Application System for GegOS
 * Built-in applications and file execution
 */

#include "apps.h"
#include "vga.h"
#include "keyboard.h"
#include "mouse.h"
#include "gui.h"
#include "wifi.h"
#include "io.h"
#include "terminal.h"

/* String comparison */
static int str_equals(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* String length */
static int str_len(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

/* Check if string ends with suffix */
static int str_endswith(const char* str, const char* suffix) {
    int str_l = str_len(str);
    int suf_l = str_len(suffix);
    if (suf_l > str_l) return 0;
    
    const char* s = str + str_l - suf_l;
    while (*s && *suffix) {
        char c1 = *s, c2 = *suffix;
        /* Case insensitive */
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return 0;
        s++; suffix++;
    }
    return 1;
}

/* Virtual file system */
static file_entry_t virtual_files[] = {
    {"readme.txt", FILE_TYPE_TXT, "Welcome to GegOS!\n\nThis is a simple hobby OS.", 48},
    {"hello.geg", FILE_TYPE_GEG, "PRINT Hello from GegOS!", 24},
    {"startup.bat", FILE_TYPE_BAT, "@echo GegOS Starting...\n@echo Ready!", 38},
    {"test.vbs", FILE_TYPE_VBS, "MsgBox \"Hello from VBScript!\"", 30},
    {"calc.exe", FILE_TYPE_EXE, "[Calculator Application]", 24},
    {"notes.txt", FILE_TYPE_TXT, "My Notes:\n- Learn OS dev\n- Have fun!", 38},
    {0, 0, 0, 0}  /* End marker */
};

/* Built-in applications */
static app_t apps[] = {
    {"Browser", "[WWW]", app_browser, 0},
    {"WiFi", "[NET]", app_wifi, 0},
    {"Files", "[DIR]", app_files, 0},
    {"Notepad", "[TXT]", app_notepad, 0},
    {"Terminal", "[CMD]", app_terminal, 0},
    {"Calculator", "[123]", app_calculator, 0},
    {"Settings", "[CFG]", app_settings, 0},
    {"About", "[?]", app_about, 0},
    {0, 0, 0, 0}  /* End marker */
};

static int num_apps = 7;

/* Settings state */
static int settings_win = -1;
static int settings_resolution = 0;  /* 0=640x480, 1=320x200, 2=1280x720 */
static int settings_mouse_speed = 1; /* 0=slow, 1=normal, 2=fast */
static int settings_theme = 0;       /* 0=cyan, 1=gray, 2=blue */

/* Browser state */
static int browser_page = 0;
static const char* browser_pages[] = {
    "GegOS Home\n\n"
    "Welcome to Potato!\n\n"
    "Links:\n"
    "[1] About GegOS\n"
    "[2] Help Page\n"
    "[3] Fun Page",
    
    "About GegOS\n\n"
    "GegOS v1.0\n"
    "A hobby operating\n"
    "system with GUI.\n\n"
    "[0] Back to Home",
    
    "Help Page\n\n"
    "Mouse: Click btns\n"
    "Keys: Q=Quit app\n"
    "Drag title bars!\n\n"
    "[0] Back to Home",
    
    "Fun Page\n\n"
    "Thanks for using\n"
    "GegOS! :)\n\n"
    "Have a great day!\n\n"
    "[0] Back to Home"
};

/* Terminal state */
static char terminal_buffer[256];
static int terminal_cursor = 0;
static char terminal_output[512];
static int terminal_out_len = 0;

/* Notepad state */
static char notepad_buffer[512];
static int notepad_cursor = 0;

/* Calculator state */
static int calc_value = 0;
static int calc_operand = 0;
static char calc_op = 0;
static char calc_display[16];

/* Initialize apps */
void apps_init(void) {
    terminal_cursor = 0;
    terminal_out_len = 0;
    notepad_cursor = 0;
    calc_value = 0;
    
    for (int i = 0; i < 256; i++) terminal_buffer[i] = 0;
    for (int i = 0; i < 512; i++) terminal_output[i] = 0;
    for (int i = 0; i < 512; i++) notepad_buffer[i] = 0;
    
    /* Initialize notepad with some text */
    const char* init_text = "Welcome to GegOS Notepad!\nType here...";
    for (int i = 0; init_text[i]; i++) {
        notepad_buffer[i] = init_text[i];
        notepad_cursor = i + 1;
    }
}

/* Get file type */
file_type_t get_file_type(const char* filename) {
    if (str_endswith(filename, ".geg")) return FILE_TYPE_GEG;
    if (str_endswith(filename, ".exe")) return FILE_TYPE_EXE;
    if (str_endswith(filename, ".bat")) return FILE_TYPE_BAT;
    if (str_endswith(filename, ".vbs")) return FILE_TYPE_VBS;
    if (str_endswith(filename, ".txt")) return FILE_TYPE_TXT;
    return FILE_TYPE_UNKNOWN;
}

/* Execute file */
int file_execute(const char* filename) {
    /* Find file */
    for (int i = 0; virtual_files[i].name; i++) {
        if (str_equals(virtual_files[i].name, filename)) {
            file_entry_t* f = &virtual_files[i];
            
            switch (f->type) {
                case FILE_TYPE_GEG:
                    /* Run native GegOS app - show content */
                    terminal_out_len = 0;
                    for (int j = 0; f->content[j] && terminal_out_len < 500; j++) {
                        terminal_output[terminal_out_len++] = f->content[j];
                    }
                    terminal_output[terminal_out_len++] = '\n';
                    return 1;
                    
                case FILE_TYPE_EXE:
                    /* Simulated EXE - launch calculator for calc.exe */
                    if (str_endswith(filename, "calc.exe")) {
                        app_calculator();
                    } else {
                        terminal_out_len = 0;
                        const char* msg = "Running EXE: ";
                        for (int j = 0; msg[j]; j++) 
                            terminal_output[terminal_out_len++] = msg[j];
                        for (int j = 0; filename[j]; j++)
                            terminal_output[terminal_out_len++] = filename[j];
                        terminal_output[terminal_out_len++] = '\n';
                    }
                    return 1;
                    
                case FILE_TYPE_BAT:
                    /* Run batch script - show commands */
                    terminal_out_len = 0;
                    const char* bat_hdr = "Running BAT:\n";
                    for (int j = 0; bat_hdr[j]; j++)
                        terminal_output[terminal_out_len++] = bat_hdr[j];
                    for (int j = 0; f->content[j] && terminal_out_len < 500; j++) {
                        terminal_output[terminal_out_len++] = f->content[j];
                    }
                    terminal_output[terminal_out_len++] = '\n';
                    return 1;
                    
                case FILE_TYPE_VBS:
                    /* Run VBScript - show message */
                    terminal_out_len = 0;
                    const char* vbs_hdr = "VBScript:\n";
                    for (int j = 0; vbs_hdr[j]; j++)
                        terminal_output[terminal_out_len++] = vbs_hdr[j];
                    for (int j = 0; f->content[j] && terminal_out_len < 500; j++) {
                        terminal_output[terminal_out_len++] = f->content[j];
                    }
                    terminal_output[terminal_out_len++] = '\n';
                    return 1;
                    
                case FILE_TYPE_TXT:
                    /* Open in notepad */
                    notepad_cursor = 0;
                    for (int j = 0; f->content[j] && notepad_cursor < 500; j++) {
                        notepad_buffer[notepad_cursor++] = f->content[j];
                    }
                    notepad_buffer[notepad_cursor] = 0;
                    app_notepad();
                    return 1;
                    
                default:
                    return 0;
            }
        }
    }
    return 0;
}

/* Get app count */
int apps_get_count(void) {
    return num_apps;
}

/* Get app by index */
app_t* apps_get(int index) {
    if (index >= 0 && index < num_apps) {
        return &apps[index];
    }
    return 0;
}

/* Run app by name */
int app_run(const char* name) {
    for (int i = 0; i < num_apps; i++) {
        if (str_equals(apps[i].name, name)) {
            apps[i].run();
            return 1;
        }
    }
    return 0;
}

/* ==================== BROWSER APP (Potato) ==================== */

static int browser_win = -1;

void app_browser(void) {
    browser_page = 0;
    browser_win = gui_create_window(120, 50, 400, 300, "Potato Browser");
    gui_set_active_window(browser_win);
}

void browser_draw_content(gui_window_t* win) {
    if (!win || !win->visible) return;
    
    int x = win->x + 3;
    int y = win->y + 17;
    
    /* Clear content area */
    vga_fillrect(x, y, win->width - 6, win->height - 20, COLOR_WHITE);
    
    /* Tab bar */
    vga_fillrect(x, y, 60, 14, COLOR_LIGHT_GRAY);
    vga_rect(x, y, 60, 14, COLOR_BLACK);
    vga_putstring(x + 4, y + 3, "+ Tab", COLOR_BLACK, COLOR_LIGHT_GRAY);
    
    y += 16;
    
    /* Navigation buttons */
    int bx = x;
    vga_fillrect(bx, y, 20, 14, COLOR_LIGHT_GRAY);
    vga_rect(bx, y, 20, 14, COLOR_BLACK);
    vga_putstring(bx + 4, y + 3, "<", COLOR_BLACK, COLOR_LIGHT_GRAY);
    
    bx += 22;
    vga_fillrect(bx, y, 20, 14, COLOR_LIGHT_GRAY);
    vga_rect(bx, y, 20, 14, COLOR_BLACK);
    vga_putstring(bx + 4, y + 3, ">", COLOR_BLACK, COLOR_LIGHT_GRAY);
    
    bx += 22;
    vga_fillrect(bx, y, 20, 14, COLOR_LIGHT_GRAY);
    vga_rect(bx, y, 20, 14, COLOR_BLACK);
    vga_putstring(bx + 4, y + 3, "R", COLOR_BLACK, COLOR_LIGHT_GRAY);
    
    /* URL bar */
    bx += 24;
    vga_fillrect(bx, y, win->width - (bx - x) - 10, 14, COLOR_WHITE);
    vga_rect(bx, y, win->width - (bx - x) - 10, 14, COLOR_BLACK);
    
    const char* urls[] = {"potato://home", "potato://search", "potato://news", "potato://games"};
    vga_putstring(bx + 4, y + 3, urls[browser_page], COLOR_DARK_GRAY, COLOR_WHITE);
    
    /* Page content */
    y += 18;
    const char* content = browser_pages[browser_page];
    int cx = x + 4, cy = y;
    
    while (*content) {
        if (*content == '\n') {
            cx = x + 4;
            cy += 10;
        } else {
            if (cx < win->x + win->width - 10) {
                vga_putchar(cx, cy, *content, COLOR_BLACK, COLOR_WHITE);
                cx += 8;
            }
        }
        content++;
    }
    
    /* Status bar */
    int status_y = win->y + win->height - 14;
    vga_fillrect(win->x + 3, status_y, win->width - 6, 12, COLOR_LIGHT_GRAY);
    vga_putstring(win->x + 8, status_y + 2, "Ready", COLOR_BLACK, COLOR_LIGHT_GRAY);
}

void browser_handle_key(char key) {
    if (key >= '0' && key <= '3') {
        browser_page = key - '0';
    }
}

/* ==================== FILES APP ==================== */

static int files_win = -1;
static int files_selected = -1;

void app_files(void) {
    files_win = gui_create_window(140, 70, 360, 280, "Files");
    files_selected = -1;
    gui_set_active_window(files_win);
}

void files_draw_content(gui_window_t* win) {
    if (!win || !win->visible) return;
    
    int x = win->x + 5;
    int y = win->y + 20;
    
    /* Clear content area */
    vga_fillrect(win->x + 3, win->y + 16, win->width - 6, win->height - 19, COLOR_WHITE);
    
    /* Draw files */
    int idx = 0;
    for (int i = 0; virtual_files[i].name; i++) {
        uint8_t bg = (i == files_selected) ? COLOR_BLUE : COLOR_WHITE;
        uint8_t fg = (i == files_selected) ? COLOR_WHITE : COLOR_BLACK;
        
        /* File icon based on type */
        const char* icon = "[?]";
        switch (virtual_files[i].type) {
            case FILE_TYPE_GEG: icon = "[G]"; break;
            case FILE_TYPE_EXE: icon = "[E]"; break;
            case FILE_TYPE_BAT: icon = "[B]"; break;
            case FILE_TYPE_VBS: icon = "[V]"; break;
            case FILE_TYPE_TXT: icon = "[T]"; break;
            default: break;
        }
        
        vga_fillrect(x, y + idx * 12, win->width - 12, 11, bg);
        vga_putstring(x + 2, y + idx * 12 + 2, icon, fg, bg);
        vga_putstring(x + 28, y + idx * 12 + 2, virtual_files[i].name, fg, bg);
        idx++;
    }
    
    /* Instructions */
    vga_putstring(x, win->y + win->height - 14, "Click file, Enter=Run", COLOR_DARK_GRAY, COLOR_WHITE);
}

void files_handle_click(gui_window_t* win, int mx, int my) {
    if (!win) return;
    
    int x = win->x + 5;
    int y = win->y + 20;
    
    for (int i = 0; virtual_files[i].name; i++) {
        if (mx >= x && mx < x + win->width - 12 &&
            my >= y + i * 12 && my < y + i * 12 + 11) {
            files_selected = i;
            return;
        }
    }
}

void files_handle_key(char key) {
    unsigned char ukey = (unsigned char)key;
    if (key == '\n' && files_selected >= 0 && virtual_files[files_selected].name) {
        file_execute(virtual_files[files_selected].name);
    }
    if (ukey == KEY_UP && files_selected > 0) {
        files_selected--;
    }
    if (ukey == KEY_DOWN && virtual_files[files_selected + 1].name) {
        files_selected++;
    }
}

/* ==================== NOTEPAD APP ==================== */

static int notepad_win = -1;
static int last_notepad_cursor = -1;

void app_notepad(void) {
    notepad_win = gui_create_window(160, 60, 380, 300, "Notepad");
    gui_set_active_window(notepad_win);
}

void notepad_draw_content(gui_window_t* win) {
    if (!win || !win->visible) return;
    
    /* Full redraw only if cursor moved backwards or content cleared */
    if (last_notepad_cursor > notepad_cursor || last_notepad_cursor == -1) {
        /* Gray frame */
        vga_fillrect(win->x + 3, win->y + 22, win->width - 6, win->height - 25, COLOR_LIGHT_GRAY);
        
        /* White text area (sunken) */
        int text_x = win->x + 6;
        int text_y = win->y + 25;
        int text_w = win->width - 12;
        int text_h = win->height - 31;
        
        vga_fillrect(text_x, text_y, text_w, text_h, COLOR_WHITE);
        /* Sunken border */
        vga_hline(text_x, text_y, text_w, COLOR_DARK_GRAY);
        vga_vline(text_x, text_y, text_h, COLOR_DARK_GRAY);
        vga_hline(text_x, text_y + text_h - 1, text_w, COLOR_WHITE);
        vga_vline(text_x + text_w - 1, text_y, text_h, COLOR_WHITE);
        
        int x = text_x + 3;
        int y = text_y + 3;
        int start_x = x;
        
        for (int i = 0; i < notepad_cursor && notepad_buffer[i]; i++) {
            if (notepad_buffer[i] == '\n') {
                x = start_x;
                y += 10;
                if (y > win->y + win->height - 20) break;
            } else {
                if (x < win->x + win->width - 10) {
                    vga_putchar(x, y, notepad_buffer[i], COLOR_BLACK, COLOR_WHITE);
                    x += 8;
                }
            }
        }
        last_notepad_cursor = notepad_cursor;
    } else if (last_notepad_cursor < notepad_cursor) {
        /* Only draw new character */
        int x = win->x + 5;
        int y = win->y + 20;
        int start_x = x;
        
        /* Calculate cursor position */
        for (int i = 0; i < last_notepad_cursor && notepad_buffer[i]; i++) {
            if (notepad_buffer[i] == '\n') {
                x = start_x;
                y += 10;
            } else {
                x += 8;
            }
        }
        
        /* Draw only the new character */
        if (last_notepad_cursor < notepad_cursor && x < win->x + win->width - 10 && y < win->y + win->height - 20) {
            char new_char = notepad_buffer[last_notepad_cursor];
            if (new_char == '\n') {
                /* Just update position tracking */
            } else {
                /* Clear old cursor */
                vga_fillrect(x, y, 10, 8, COLOR_WHITE);
                vga_putchar(x, y, new_char, COLOR_BLACK, COLOR_WHITE);
                x += 8;
            }
        }
        last_notepad_cursor = notepad_cursor;
    }
    
    /* Draw cursor at current position */
    int x = win->x + 5;
    int y = win->y + 20;
    int start_x = x;
    
    for (int i = 0; i < notepad_cursor && notepad_buffer[i]; i++) {
        if (notepad_buffer[i] == '\n') {
            x = start_x;
            y += 10;
        } else {
            x += 8;
        }
    }
    
    if (x < win->x + win->width - 10 && y < win->y + win->height - 10) {
        vga_fillrect(x, y, 2, 8, COLOR_BLACK);
    }
}

void notepad_handle_key(char key) {
    if (key == '\b') {
        if (notepad_cursor > 0) {
            notepad_cursor--;
            notepad_buffer[notepad_cursor] = 0;
        }
    } else if (key >= 32 && key < 127) {
        if (notepad_cursor < 500) {
            notepad_buffer[notepad_cursor++] = key;
            notepad_buffer[notepad_cursor] = 0;
        }
    } else if (key == '\n') {
        if (notepad_cursor < 500) {
            notepad_buffer[notepad_cursor++] = '\n';
            notepad_buffer[notepad_cursor] = 0;
        }
    }
}

/* ==================== TERMINAL APP ==================== */

static int terminal_win = -1;
static int terminal_initialized = 0;

void app_terminal(void) {
    terminal_win = gui_create_window(50, 50, 500, 350, "Terminal - bash");
    if (!terminal_initialized) {
        terminal_init();
        terminal_initialized = 1;
    }
    gui_set_active_window(terminal_win);
}

void terminal_draw_content(gui_window_t* win) {
    if (!win || !win->visible) return;
    terminal_draw(win->x, win->y + 15, win->width, win->height - 15);
}

void terminal_key_handler(char key) {
    terminal_handle_key(key);
}

/* ==================== CALCULATOR APP ==================== */

static int calc_win = -1;
static char last_calc_display[16] = "";

void app_calculator(void) {
    calc_win = gui_create_window(200, 100, 160, 200, "Calc");
    calc_value = 0;
    calc_operand = 0;
    calc_op = 0;
    calc_display[0] = '0';
    calc_display[1] = 0;
    gui_set_active_window(calc_win);
}

void calc_draw_content(gui_window_t* win) {
    if (!win || !win->visible) return;
    
    int x = win->x + 5;
    int y = win->y + 20;
    
    /* Only redraw display if it changed */
    int display_changed = 0;
    for (int i = 0; i < 16; i++) {
        if (last_calc_display[i] != calc_display[i]) {
            display_changed = 1;
            break;
        }
    }
    
    if (display_changed || last_calc_display[0] == 0) {
        /* Clear and redraw only display area */
        vga_fillrect(x, y, win->width - 12, 16, COLOR_WHITE);
        vga_rect(x, y, win->width - 12, 16, COLOR_BLACK);
        vga_putstring(x + 4, y + 4, calc_display, COLOR_BLACK, COLOR_WHITE);
        
        /* Save current display */
        for (int i = 0; i < 16; i++) {
            last_calc_display[i] = calc_display[i];
        }
    }
    
    /* Draw buttons only on first draw */
    static int buttons_drawn = 0;
    if (!buttons_drawn || win->dirty_region.dirty) {
        /* Clear content area */
        vga_fillrect(win->x + 3, win->y + 16, win->width - 6, win->height - 19, COLOR_LIGHT_GRAY);
        
        /* Redraw display */
        vga_fillrect(x, y, win->width - 12, 16, COLOR_WHITE);
        vga_rect(x, y, win->width - 12, 16, COLOR_BLACK);
        vga_putstring(x + 4, y + 4, calc_display, COLOR_BLACK, COLOR_WHITE);
        
        /* Buttons */
        const char* btns[] = {"7", "8", "9", "+", "4", "5", "6", "-", "1", "2", "3", "*", "C", "0", "=", "/"};
        int bx = x, by = y + 20;
        
        for (int i = 0; i < 16; i++) {
            vga_fillrect(bx, by, 18, 16, COLOR_WHITE);
            vga_rect(bx, by, 18, 16, COLOR_BLACK);
            vga_putchar(bx + 5, by + 4, btns[i][0], COLOR_BLACK, COLOR_WHITE);
            
            bx += 22;
            if ((i + 1) % 4 == 0) {
                bx = x;
                by += 18;
            }
        }
        buttons_drawn = 1;
    }
}

static void calc_update_display(void) {
    int v = calc_value;
    int neg = 0;
    if (v < 0) { neg = 1; v = -v; }
    
    int i = 14;
    calc_display[15] = 0;
    
    if (v == 0) {
        calc_display[i--] = '0';
    } else {
        while (v > 0 && i >= 0) {
            calc_display[i--] = '0' + (v % 10);
            v /= 10;
        }
    }
    if (neg && i >= 0) calc_display[i--] = '-';
    
    /* Shift to start */
    int start = i + 1;
    for (int j = 0; j <= 15 - start; j++) {
        calc_display[j] = calc_display[start + j];
    }
}

void calc_handle_click(gui_window_t* win, int mx, int my) {
    if (!win) return;
    
    int x = win->x + 5;
    int y = win->y + 40;
    
    const char ops[] = "789+456-123*C0=/";
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int bx = x + col * 22;
            int by = y + row * 18;
            
            if (mx >= bx && mx < bx + 18 && my >= by && my < by + 16) {
                char c = ops[row * 4 + col];
                
                if (c >= '0' && c <= '9') {
                    calc_value = calc_value * 10 + (c - '0');
                    calc_update_display();
                } else if (c == 'C') {
                    calc_value = 0;
                    calc_operand = 0;
                    calc_op = 0;
                    calc_update_display();
                } else if (c == '=') {
                    switch (calc_op) {
                        case '+': calc_value = calc_operand + calc_value; break;
                        case '-': calc_value = calc_operand - calc_value; break;
                        case '*': calc_value = calc_operand * calc_value; break;
                        case '/': if (calc_value != 0) calc_value = calc_operand / calc_value; break;
                    }
                    calc_op = 0;
                    calc_update_display();
                } else {
                    calc_operand = calc_value;
                    calc_value = 0;
                    calc_op = c;
                }
                return;
            }
        }
    }
}

void calc_handle_key(char key) {
    if (key >= '0' && key <= '9') {
        calc_value = calc_value * 10 + (key - '0');
        calc_update_display();
    } else if (key == 'c' || key == 'C') {
        calc_value = 0;
        calc_operand = 0;
        calc_op = 0;
        calc_update_display();
    } else if (key == '\n' || key == '=') {
        switch (calc_op) {
            case '+': calc_value = calc_operand + calc_value; break;
            case '-': calc_value = calc_operand - calc_value; break;
            case '*': calc_value = calc_operand * calc_value; break;
            case '/': if (calc_value != 0) calc_value = calc_operand / calc_value; break;
        }
        calc_op = 0;
        calc_update_display();
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        calc_operand = calc_value;
        calc_value = 0;
        calc_op = key;
    }
}

/* ==================== ABOUT APP ==================== */

static int about_win = -1;

void app_about(void) {
    about_win = gui_create_window(180, 120, 280, 180, "About GegOS");
    gui_set_active_window(about_win);
}

void about_draw_content(gui_window_t* win) {
    if (!win || !win->visible) return;
    
    /* Clear content area */
    vga_fillrect(win->x + 3, win->y + 16, win->width - 6, win->height - 19, COLOR_WHITE);
    
    int x = win->x + 10;
    int y = win->y + 25;
    
    /* Logo */
    vga_fillrect(x, y, 32, 32, COLOR_BLUE);
    vga_putstring(x + 4, y + 12, "Geg", COLOR_WHITE, COLOR_BLUE);
    
    /* Info */
    vga_putstring(x + 40, y, "GegOS v1.0", COLOR_BLACK, COLOR_WHITE);
    vga_putstring(x + 80, y, " v1.0", COLOR_RED, COLOR_WHITE);
    vga_putstring(x + 40, y + 12, "Hobby OS", COLOR_DARK_GRAY, COLOR_WHITE);
    vga_putstring(x + 40, y + 24, "2026", COLOR_DARK_GRAY, COLOR_WHITE);
    
    vga_putstring(x, y + 45, "Simple GUI OS with", COLOR_BLACK, COLOR_WHITE);
    vga_putstring(x, y + 55, "mouse & keyboard", COLOR_BLACK, COLOR_WHITE);
}

/* ==================== SETTINGS APP ==================== */

void app_settings(void) {
    settings_win = gui_create_window(150, 80, 320, 280, "Settings");
    gui_set_active_window(settings_win);
}

void settings_draw_content(gui_window_t* win) {
    if (!win || !win->visible) return;
    
    /* Clear content area */
    vga_fillrect(win->x + 3, win->y + 16, win->width - 6, win->height - 19, COLOR_WHITE);
    
    int x = win->x + 8;
    int y = win->y + 22;
    
    /* Resolution setting */
    vga_putstring(x, y, "Resolution:", COLOR_BLACK, COLOR_WHITE);
    
    /* Resolution options */
    const char* res_opts[] = {"640x480", "320x200", "1280x720"};
    for (int i = 0; i < 3; i++) {
        int bx = x + 75 + i * 50;
        uint8_t bg = (settings_resolution == i) ? COLOR_BLUE : COLOR_LIGHT_GRAY;
        uint8_t fg = (settings_resolution == i) ? COLOR_WHITE : COLOR_BLACK;
        vga_fillrect(bx, y - 2, 48, 12, bg);
        vga_rect(bx, y - 2, 48, 12, COLOR_BLACK);
        vga_putstring(bx + 2, y, res_opts[i], fg, bg);
    }
    
    y += 32;
    
    /* Mouse speed setting */
    vga_putstring(x, y, "Mouse:", COLOR_BLACK, COLOR_WHITE);
    
    const char* speed_opts[] = {"Slow", "Med", "Fast"};
    for (int i = 0; i < 3; i++) {
        int bx = x + 75 + i * 36;
        uint8_t bg = (settings_mouse_speed == i) ? COLOR_BLUE : COLOR_LIGHT_GRAY;
        uint8_t fg = (settings_mouse_speed == i) ? COLOR_WHITE : COLOR_BLACK;
        vga_fillrect(bx, y - 2, 34, 12, bg);
        vga_rect(bx, y - 2, 34, 12, COLOR_BLACK);
        vga_putstring(bx + 3, y, speed_opts[i], fg, bg);
    }
    
    y += 28;
    
    /* Theme setting */
    vga_putstring(x, y, "Theme:", COLOR_BLACK, COLOR_WHITE);
    
    uint8_t theme_colors[] = {COLOR_CYAN, COLOR_LIGHT_GRAY, COLOR_BLUE};
    for (int i = 0; i < 3; i++) {
        int bx = x + 75 + i * 36;
        vga_fillrect(bx, y - 2, 34, 12, theme_colors[i]);
        vga_rect(bx, y - 2, 34, 12, (settings_theme == i) ? COLOR_WHITE : COLOR_BLACK);
        if (settings_theme == i) {
            vga_rect(bx + 1, y - 1, 32, 10, COLOR_BLACK);
        }
    }
    
    y += 28;
    
    /* Info */
    vga_putstring(x, y, "Click options to change", COLOR_DARK_GRAY, COLOR_WHITE);
}

void settings_handle_click(gui_window_t* win, int mx, int my) {
    if (!win) return;
    
    int x = win->x + 8;
    int y = win->y + 22;
    
    /* Check resolution buttons */
    for (int i = 0; i < 3; i++) {
        int bx = x + 75 + i * 50;
        if (mx >= bx && mx < bx + 48 && my >= y - 2 && my < y + 10) {
            settings_resolution = i;
            vga_set_mode(i);  /* Actually apply the resolution change */
            return;
        }
    }
    
    y += 32;
    
    /* Check mouse speed buttons */
    for (int i = 0; i < 3; i++) {
        int bx = x + 75 + i * 36;
        if (mx >= bx && mx < bx + 34 && my >= y - 2 && my < y + 10) {
            settings_mouse_speed = i;
            return;
        }
    }
    
    y += 28;
    
    /* Check theme buttons */
    for (int i = 0; i < 3; i++) {
        int bx = x + 75 + i * 36;
        if (mx >= bx && mx < bx + 34 && my >= y - 2 && my < y + 10) {
            settings_theme = i;
            return;
        }
    }
}

int get_settings_theme(void) { return settings_theme; }
int get_settings_mouse_speed(void) { return settings_mouse_speed; }

/* Global getters for window IDs */
int get_browser_win(void) { return browser_win; }
int get_files_win(void) { return files_win; }
int get_notepad_win(void) { return notepad_win; }
int get_terminal_win(void) { return terminal_win; }
int get_calc_win(void) { return calc_win; }
int get_about_win(void) { return about_win; }
int get_settings_win(void) { return settings_win; }
