/*
 * vga.h - VGA Graphics Driver for GegOS
 * 640x480, 16 colors (Mode 12h)
 */

#ifndef VGA_H
#define VGA_H

#include <stdint.h>

/* Screen dimensions - Mode 12h */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* VGA Color Palette (standard 256-color palette) */
#define COLOR_BLACK       0
#define COLOR_BLUE        1
#define COLOR_GREEN       2
#define COLOR_CYAN        3
#define COLOR_RED         4
#define COLOR_MAGENTA     5
#define COLOR_BROWN       6
#define COLOR_LIGHT_GRAY  7
#define COLOR_DARK_GRAY   8
#define COLOR_LIGHT_BLUE  9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN  11
#define COLOR_LIGHT_RED   12
#define COLOR_PINK        13
#define COLOR_YELLOW      14
#define COLOR_WHITE       15

/* Initialize VGA Mode 13h (320x200, 256 colors) */
void vga_init(void);

/* Clear screen with a color */
void vga_clear(uint8_t color);

/* Draw a single pixel */
void vga_putpixel(int x, int y, uint8_t color);

/* Get pixel color at position */
uint8_t vga_getpixel(int x, int y);

/* Draw a horizontal line */
void vga_hline(int x, int y, int width, uint8_t color);

/* Draw a vertical line */
void vga_vline(int x, int y, int height, uint8_t color);

/* Draw a line (Bresenham's algorithm) */
void vga_line(int x1, int y1, int x2, int y2, uint8_t color);

/* Draw a rectangle outline */
void vga_rect(int x, int y, int width, int height, uint8_t color);

/* Draw a filled rectangle */
void vga_fillrect(int x, int y, int width, int height, uint8_t color);

/* Draw a circle outline */
void vga_circle(int cx, int cy, int radius, uint8_t color);

/* Draw a filled circle */
void vga_fillcircle(int cx, int cy, int radius, uint8_t color);

/* Draw a character at position (8x8 bitmap font) */
void vga_putchar(int x, int y, char c, uint8_t fg, uint8_t bg);

/* Draw a string at position */
void vga_putstring(int x, int y, const char* str, uint8_t fg, uint8_t bg);

/* Draw a bitmap */
void vga_drawbitmap(int x, int y, int width, int height, const uint8_t* bitmap);

/* Copy screen region (for mouse cursor) */
void vga_copyrect(int sx, int sy, int dx, int dy, int w, int h);

/* Swap double buffer (if using) */
void vga_swap(void);

/* Wait for vertical retrace (smooth animation) */
void vga_vsync(void);

/* Set VGA mode (0=640x480, 1=320x200) */
void vga_set_mode(int mode);

/* Get current VGA mode */
int vga_get_mode(void);

#endif /* VGA_H */
