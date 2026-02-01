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
static int needs_full_draw = 1;
static segment_t old_tail;  /* Track old tail for erasing */
static int old_food_x = -1, old_food_y = -1;

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
    needs_full_draw = 1;
    old_food_x = -1;
    old_food_y = -1;
}

void snake_update(void) {
    if (game_over) return;
    
    /* Update direction */
    if ((snake.next_dir_x != -snake.dir_x || snake.next_dir_y != -snake.dir_y)) {
        snake.dir_x = snake.next_dir_x;
        snake.dir_y = snake.next_dir_y;
    }
    
    /* Save old tail position before moving */
    old_tail = snake.body[snake.length - 1];
    
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
            /* New segment at old tail position */
            snake.body[snake.length - 1] = old_tail;
        }
        score += 10;
        
        /* Save old food position and generate new */
        old_food_x = food.x;
        old_food_y = food.y;
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
    if (needs_full_draw) {
        /* Full redraw - only on first frame */
        vga_fillrect(0, 0, GAME_WIDTH, GAME_HEIGHT, COLOR_BLACK);
        
        /* Draw grid (light grid lines) */
        for (int i = 0; i < GAME_WIDTH; i += CELL_SIZE) {
            vga_vline(i, 0, GAME_HEIGHT, COLOR_DARK_GRAY);
        }
        for (int i = 0; i < GAME_HEIGHT; i += CELL_SIZE) {
            vga_hline(0, i, GAME_WIDTH, COLOR_DARK_GRAY);
        }
        
        /* Draw entire snake */
        for (int i = 0; i < snake.length; i++) {
            uint8_t color = (i == 0) ? COLOR_GREEN : COLOR_LIGHT_GREEN;
            int px = snake.body[i].x * CELL_SIZE + 1;
            int py = snake.body[i].y * CELL_SIZE + 1;
            vga_fillrect(px, py, CELL_SIZE - 2, CELL_SIZE - 2, color);
        }
        
        /* Draw score and status labels */
        int status_y = GAME_HEIGHT + 10;
        vga_putstring(10, status_y, "Score: ", COLOR_WHITE, COLOR_BLACK);
        vga_putstring(10, status_y + 15, "Length: ", COLOR_WHITE, COLOR_BLACK);
        
        needs_full_draw = 0;
    } else {
        /* Partial redraw - only erase old tail, draw new head */
        
        /* Erase old tail cell (draw black + grid lines) */
        int tx = old_tail.x * CELL_SIZE;
        int ty = old_tail.y * CELL_SIZE;
        vga_fillrect(tx, ty, CELL_SIZE, CELL_SIZE, COLOR_BLACK);
        vga_vline(tx, ty, CELL_SIZE, COLOR_DARK_GRAY);
        vga_hline(tx, ty, CELL_SIZE, COLOR_DARK_GRAY);
        
        /* Erase old food if it was eaten */
        if (old_food_x >= 0) {
            int ofx = old_food_x * CELL_SIZE;
            int ofy = old_food_y * CELL_SIZE;
            vga_fillrect(ofx, ofy, CELL_SIZE, CELL_SIZE, COLOR_BLACK);
            vga_vline(ofx, ofy, CELL_SIZE, COLOR_DARK_GRAY);
            vga_hline(ofx, ofy, CELL_SIZE, COLOR_DARK_GRAY);
            old_food_x = -1;
            old_food_y = -1;
        }
        
        /* Draw new head (green) */
        int hx = snake.body[0].x * CELL_SIZE + 1;
        int hy = snake.body[0].y * CELL_SIZE + 1;
        vga_fillrect(hx, hy, CELL_SIZE - 2, CELL_SIZE - 2, COLOR_GREEN);
        
        /* Redraw old head as body (light green) */
        if (snake.length > 1) {
            int ox = snake.body[1].x * CELL_SIZE + 1;
            int oy = snake.body[1].y * CELL_SIZE + 1;
            vga_fillrect(ox, oy, CELL_SIZE - 2, CELL_SIZE - 2, COLOR_LIGHT_GREEN);
        }
    }
    
    /* Always draw food */
    int fx = food.x * CELL_SIZE + 2;
    int fy = food.y * CELL_SIZE + 2;
    vga_fillrect(fx, fy, CELL_SIZE - 4, CELL_SIZE - 4, COLOR_RED);
    
    if (game_over) {
        vga_putstring(50, GAME_HEIGHT + 40, "GAME OVER! Press SPACE", COLOR_RED, COLOR_BLACK);
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
        case (char)128:  /* Up arrow KEY_UP */
            if (snake.dir_y != 1) {
                snake.next_dir_x = 0;
                snake.next_dir_y = -1;
            }
            break;
        case (char)129:  /* Down arrow KEY_DOWN */
            if (snake.dir_y != -1) {
                snake.next_dir_x = 0;
                snake.next_dir_y = 1;
            }
            break;
        case (char)130:  /* Left arrow KEY_LEFT */
            if (snake.dir_x != 1) {
                snake.next_dir_x = -1;
                snake.next_dir_y = 0;
            }
            break;
        case (char)131:  /* Right arrow KEY_RIGHT */
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
        for (volatile int i = 0; i < 800000; i++);
    }
}
