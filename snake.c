/*
 * snake.c - Simple Snake Game for GegOS
 * Classic snake game with keyboard controls
 */

#include "vga.h"
#include "keyboard.h"
#include "mouse.h"
#include "io.h"

#define GRID_SIZE    20   /* 20x20 grid */
#define CELL_SIZE    10   /* Each cell is 10x10 pixels */
#define GAME_WIDTH   (GRID_SIZE * CELL_SIZE)
#define GAME_HEIGHT  (GRID_SIZE * CELL_SIZE)
#define MAX_SNAKE    200

typedef struct {
    int x, y;  /* Grid coordinates */
} segment_t;

typedef struct {
    segment_t body[MAX_SNAKE];
    int length;
    int dir_x, dir_y;
    int next_dir_x, next_dir_y;
} snake_t;

typedef struct {
    int x, y;  /* Grid coordinates */
} food_t;

static snake_t snake;
static food_t food;
static int score = 0;
static int game_over = 0;

void snake_init(void) {
    /* Start with snake in middle */
    snake.length = 3;
    snake.body[0].x = GRID_SIZE / 2;
    snake.body[0].y = GRID_SIZE / 2;
    snake.body[1].x = GRID_SIZE / 2 - 1;
    snake.body[1].y = GRID_SIZE / 2;
    snake.body[2].x = GRID_SIZE / 2 - 2;
    snake.body[2].y = GRID_SIZE / 2;
    
    snake.dir_x = 1;
    snake.dir_y = 0;
    snake.next_dir_x = 1;
    snake.next_dir_y = 0;
    
    /* Random food position */
    food.x = (7 * 13) % GRID_SIZE;
    food.y = (11 * 17) % GRID_SIZE;
    
    score = 0;
    game_over = 0;
}

void snake_update(void) {
    if (game_over) return;
    
    /* Update direction */
    if ((snake.next_dir_x != -snake.dir_x || snake.next_dir_y != -snake.dir_y)) {
        snake.dir_x = snake.next_dir_x;
        snake.dir_y = snake.next_dir_y;
    }
    
    /* Move snake - shift body and add new head */
    for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
    }
    
    snake.body[0].x += snake.dir_x;
    snake.body[0].y += snake.dir_y;
    
    /* Wrap around edges */
    if (snake.body[0].x < 0) snake.body[0].x = GRID_SIZE - 1;
    if (snake.body[0].x >= GRID_SIZE) snake.body[0].x = 0;
    if (snake.body[0].y < 0) snake.body[0].y = GRID_SIZE - 1;
    if (snake.body[0].y >= GRID_SIZE) snake.body[0].y = 0;
    
    /* Check food collision */
    if (snake.body[0].x == food.x && snake.body[0].y == food.y) {
        /* Grow snake */
        if (snake.length < MAX_SNAKE - 1) {
            snake.length++;
        }
        score += 10;
        
        /* New food position */
        food.x = (food.x * 7 + 13) % GRID_SIZE;
        food.y = (food.y * 11 + 17) % GRID_SIZE;
    }
    
    /* Check self collision */
    for (int i = 1; i < snake.length; i++) {
        if (snake.body[0].x == snake.body[i].x && 
            snake.body[0].y == snake.body[i].y) {
            game_over = 1;
        }
    }
}

void snake_draw(void) {
    /* Clear play area with dark background */
    vga_fillrect(0, 0, GAME_WIDTH, GAME_HEIGHT, COLOR_BLACK);
    
    /* Draw grid (light grid lines) */
    for (int i = 0; i < GAME_WIDTH; i += CELL_SIZE) {
        vga_vline(i, 0, GAME_HEIGHT, COLOR_DARK_GRAY);
    }
    for (int i = 0; i < GAME_HEIGHT; i += CELL_SIZE) {
        vga_hline(0, i, GAME_WIDTH, COLOR_DARK_GRAY);
    }
    
    /* Draw snake */
    for (int i = 0; i < snake.length; i++) {
        uint8_t color = (i == 0) ? COLOR_GREEN : COLOR_LIGHT_GREEN;
        int px = snake.body[i].x * CELL_SIZE + 1;
        int py = snake.body[i].y * CELL_SIZE + 1;
        vga_fillrect(px, py, CELL_SIZE - 2, CELL_SIZE - 2, color);
    }
    
    /* Draw food */
    int fx = food.x * CELL_SIZE + 2;
    int fy = food.y * CELL_SIZE + 2;
    vga_fillrect(fx, fy, CELL_SIZE - 4, CELL_SIZE - 4, COLOR_RED);
    
    /* Draw score and status */
    int status_y = GAME_HEIGHT + 10;
    vga_putstring(10, status_y, "Score: ", COLOR_WHITE, COLOR_BLACK);
    vga_putstring(10, status_y + 15, "Length: ", COLOR_WHITE, COLOR_BLACK);
    
    if (game_over) {
        vga_putstring(50, status_y + 30, "GAME OVER! Press SPACE", COLOR_RED, COLOR_BLACK);
    }
}

void snake_handle_key(char key) {
    switch (key) {
        case 'w':
        case 'W':
            if (snake.dir_y != 1) {
                snake.next_dir_x = 0;
                snake.next_dir_y = -1;
            }
            break;
        case 's':
        case 'S':
            if (snake.dir_y != -1) {
                snake.next_dir_x = 0;
                snake.next_dir_y = 1;
            }
            break;
        case 'a':
        case 'A':
            if (snake.dir_x != 1) {
                snake.next_dir_x = -1;
                snake.next_dir_y = 0;
            }
            break;
        case 'd':
        case 'D':
            if (snake.dir_x != -1) {
                snake.next_dir_x = 1;
                snake.next_dir_y = 0;
            }
            break;
        case 0x48:  /* Up arrow */
            if (snake.dir_y != 1) {
                snake.next_dir_x = 0;
                snake.next_dir_y = -1;
            }
            break;
        case 0x50:  /* Down arrow */
            if (snake.dir_y != -1) {
                snake.next_dir_x = 0;
                snake.next_dir_y = 1;
            }
            break;
        case 0x4b:  /* Left arrow */
            if (snake.dir_x != 1) {
                snake.next_dir_x = -1;
                snake.next_dir_y = 0;
            }
            break;
        case 0x4d:  /* Right arrow */
            if (snake.dir_x != -1) {
                snake.next_dir_x = 1;
                snake.next_dir_y = 0;
            }
            break;
    }
}

void snake_run(void) {
    snake_init();
    int frame_count = 0;
    
    while (!game_over && frame_count < 6000) {
        /* Handle input */
        while (keyboard_haskey()) {
            char key = keyboard_getchar();
            if (key == ' ') {
                return;
            }
            snake_handle_key(key);
        }
        
        /* Update and draw */
        if (frame_count % 5 == 0) {  /* Update every 5 frames for speed */
            snake_update();
        }
        snake_draw();
        
        frame_count++;
        
        /* Frame delay */
        for (volatile int i = 0; i < 20000; i++);
    }
}
