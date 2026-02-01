/*
 * wifi.c - WiFi Manager App for GegOS
 */

#include "wifi.h"
#include "network.h"
#include "vga.h"
#include "keyboard.h"

/* Simple string copy */
static void strcpy_safe(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static int wifi_win = -1;

typedef enum {
    WIFI_STATE_MENU,
    WIFI_STATE_SCANNING,
    WIFI_STATE_LIST,
    WIFI_STATE_PASSWORD
} wifi_state_t;

static wifi_state_t wifi_state = WIFI_STATE_MENU;
static int selected_network = 0;
static char password_input[32] = "";
static int password_cursor = 0;
static char selected_ssid[32] = "";

void app_wifi(void) {
    wifi_win = gui_create_window(400, 80, 350, 280, "WiFi Manager");
    gui_set_active_window(wifi_win);
    wifi_state = WIFI_STATE_MENU;
    selected_network = 0;
    password_input[0] = 0;
    password_cursor = 0;
}

int get_wifi_win(void) {
    return wifi_win;
}

void wifi_draw_content(gui_window_t* win) {
    if (!win || !win->visible) return;
    
    int x = win->x + 3;
    int y = win->y + 17;
    int w = win->width - 6;
    int h = win->height - 20;
    
    /* Clear content */
    vga_fillrect(x, y, w, h, COLOR_WHITE);
    
    if (wifi_state == WIFI_STATE_MENU) {
        /* Main menu */
        vga_putstring(x + 10, y + 20, "WiFi Manager", COLOR_BLACK, COLOR_WHITE);
        vga_putstring(x + 10, y + 50, "Status: ", COLOR_BLACK, COLOR_WHITE);
        vga_putstring(x + 80, y + 50, network_get_status_string(), COLOR_BLUE, COLOR_WHITE);
        
        if (network_is_connected()) {
            vga_putstring(x + 10, y + 65, "Network: ", COLOR_BLACK, COLOR_WHITE);
            vga_putstring(x + 80, y + 65, network_get_connected_name(), COLOR_GREEN, COLOR_WHITE);
        }
        
        /* Buttons */
        vga_fillrect(x + 20, y + 100, 120, 20, COLOR_LIGHT_GRAY);
        vga_rect(x + 20, y + 100, 120, 20, COLOR_BLACK);
        vga_putstring(x + 40, y + 105, "Scan Networks", COLOR_BLACK, COLOR_LIGHT_GRAY);
        
        vga_fillrect(x + 20, y + 130, 120, 20, COLOR_LIGHT_GRAY);
        vga_rect(x + 20, y + 130, 120, 20, COLOR_BLACK);
        vga_putstring(x + 35, y + 135, "Disconnect", COLOR_BLACK, COLOR_LIGHT_GRAY);
        
    } else if (wifi_state == WIFI_STATE_SCANNING) {
        vga_putstring(x + 50, y + 100, "Scanning for", COLOR_BLACK, COLOR_WHITE);
        vga_putstring(x + 50, y + 120, "WiFi networks...", COLOR_BLACK, COLOR_WHITE);
        network_scan_wifi();
        wifi_state = WIFI_STATE_LIST;
        
    } else if (wifi_state == WIFI_STATE_LIST) {
        vga_putstring(x + 10, y + 10, "Available Networks:", COLOR_BLACK, COLOR_WHITE);
        
        int net_count = 0;
        wifi_network_t* networks = network_get_networks(&net_count);
        
        for (int i = 0; i < net_count; i++) {
            int item_y = y + 30 + (i * 35);
            
            uint8_t bg = (i == selected_network) ? COLOR_BLUE : COLOR_LIGHT_GRAY;
            uint8_t fg = (i == selected_network) ? COLOR_WHITE : COLOR_BLACK;
            
            vga_fillrect(x + 10, item_y, w - 20, 30, bg);
            vga_rect(x + 10, item_y, w - 20, 30, COLOR_BLACK);
            
            vga_putstring(x + 15, item_y + 5, networks[i].ssid, fg, bg);
            
            char signal[10];
            signal[0] = '[';
            int bars = (networks[i].signal_strength / 25);
            for (int b = 0; b < 4; b++) {
                signal[b+1] = (b < bars) ? '=' : '-';
            }
            signal[5] = ']';
            signal[6] = 0;
            vga_putstring(x + w - 80, item_y + 5, signal, fg, bg);
        }
        
        vga_putstring(x + 10, y + h - 25, "UP/DOWN: Select | ENTER: Connect", COLOR_DARK_GRAY, COLOR_WHITE);
        
    } else if (wifi_state == WIFI_STATE_PASSWORD) {
        vga_putstring(x + 10, y + 20, "Enter Password for:", COLOR_BLACK, COLOR_WHITE);
        vga_putstring(x + 10, y + 40, selected_ssid, COLOR_BLUE, COLOR_WHITE);
        
        vga_putstring(x + 10, y + 70, "Password: ", COLOR_BLACK, COLOR_WHITE);
        
        /* Password input box */
        vga_fillrect(x + 10, y + 90, w - 20, 20, COLOR_WHITE);
        vga_rect(x + 10, y + 90, w - 20, 20, COLOR_BLACK);
        
        /* Draw asterisks for password */
        int px = x + 15;
        for (int i = 0; i < password_cursor; i++) {
            vga_putchar(px, y + 97, '*', COLOR_BLACK, COLOR_WHITE);
            px += 8;
        }
        
        /* Draw cursor */
        vga_putchar(px, y + 97, '_', COLOR_BLACK, COLOR_WHITE);
        
        /* Show error if any */
        if (network_get_status() == NET_ERROR) {
            vga_putstring(x + 10, y + 130, "Error: ", COLOR_RED, COLOR_WHITE);
            vga_putstring(x + 70, y + 130, network_get_error(), COLOR_RED, COLOR_WHITE);
        }
        
        vga_putstring(x + 10, y + h - 25, "ENTER: Connect | ESC: Back", COLOR_DARK_GRAY, COLOR_WHITE);
    }
}

void wifi_handle_key(char key) {
    if (wifi_state == WIFI_STATE_LIST) {
        int net_count = 0;
        network_get_networks(&net_count);
        
        if (key == (char)128) {  /* KEY_UP */
            selected_network--;
            if (selected_network < 0) selected_network = net_count - 1;
        } else if (key == (char)129) {  /* KEY_DOWN */
            selected_network++;
            if (selected_network >= net_count) selected_network = 0;
        } else if (key == '\n') {  /* ENTER */
            wifi_network_t* networks = network_get_networks(&net_count);
            strcpy_safe(selected_ssid, networks[selected_network].ssid, 32);
            
            if (networks[selected_network].requires_password) {
                wifi_state = WIFI_STATE_PASSWORD;
                password_input[0] = 0;
                password_cursor = 0;
            } else {
                /* No password needed */
                network_connect_wifi(selected_ssid, "");
                wifi_state = WIFI_STATE_MENU;
            }
        }
    } else if (wifi_state == WIFI_STATE_PASSWORD) {
        if (key == 27) {  /* ESC */
            wifi_state = WIFI_STATE_LIST;
            password_input[0] = 0;
            password_cursor = 0;
        } else if (key == 8) {  /* BACKSPACE */
            if (password_cursor > 0) {
                password_cursor--;
                password_input[password_cursor] = 0;
            }
        } else if (key == '\n') {  /* ENTER */
            password_input[password_cursor] = 0;
            network_connect_wifi(selected_ssid, password_input);
            
            if (network_is_connected()) {
                wifi_state = WIFI_STATE_MENU;
                password_input[0] = 0;
                password_cursor = 0;
            }
            /* Error message will be shown by network_get_error() */
        } else if (key >= 32 && key < 127 && password_cursor < 31) {
            password_input[password_cursor] = key;
            password_cursor++;
        }
    }
}

void wifi_handle_click(int x, int y) {
    if (wifi_state == WIFI_STATE_MENU) {
        /* Check "Scan Networks" button */
        if (x >= 20 && x <= 140 && y >= 100 && y <= 120) {
            wifi_state = WIFI_STATE_SCANNING;
        }
        /* Check "Disconnect" button */
        if (x >= 20 && x <= 140 && y >= 130 && y <= 150) {
            network_disconnect();
        }
    }
}
