/*
 * wifi.h - WiFi Manager App for GegOS
 */

#ifndef WIFI_H
#define WIFI_H

#include "gui.h"

void app_wifi(void);
void wifi_draw_content(gui_window_t* win);
void wifi_handle_key(char key);
void wifi_handle_click(int x, int y);

int get_wifi_win(void);

#endif /* WIFI_H */
