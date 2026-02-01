/*
 * terminal.h - Terminal/Shell header
 */

#ifndef TERMINAL_H
#define TERMINAL_H

void terminal_init(void);
void terminal_handle_key(char c);
void terminal_draw(int x, int y, int width, int height);
void terminal_scroll_up(void);
void terminal_scroll_down(void);

#endif
