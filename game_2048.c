/*
 * game_2048.c - 2048 Game for GegOS
 * Tile merging puzzle game
 */

#include "vga.h"
#include "keyboard.h"
#include "io.h"

#define GRID_SIZE 4
#define TILE_SIZE 30

typedef struct {
    int tiles[GRID_SIZE][GRID_SIZE];
    int old_tiles[GRID_SIZE][GRID_SIZE];  /* Track previous state */
    int score;
    int moved;
} game_state_t;

static game_state_t game;
static int needs_full_draw = 1;

void game_2048_init(void) {
    /* Clear grid */
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            game.tiles[i][j] = 0;
            game.old_tiles[i][j] = -1;  /* Force first draw */
        }
    }
    game.score = 0;
    needs_full_draw = 1;
    
    /* Add two random tiles */
    game.tiles[1][1] = 2;
    game.tiles[2][2] = 2;
}

int game_2048_is_game_over(void) {
    /* Check for empty cells */
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game.tiles[i][j] == 0) return 0;
        }
    }
    
    /* Check for possible merges */
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int current = game.tiles[i][j];
            if (j < GRID_SIZE - 1 && game.tiles[i][j + 1] == current) return 0;
            if (i < GRID_SIZE - 1 && game.tiles[i + 1][j] == current) return 0;
        }
    }
    
    return 1;
}

void game_2048_move_left(void) {
    game.moved = 0;
    
    for (int i = 0; i < GRID_SIZE; i++) {
        /* Compact */
        int write_pos = 0;
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game.tiles[i][j] != 0) {
                game.tiles[i][write_pos] = game.tiles[i][j];
                if (write_pos != j) game.moved = 1;
                write_pos++;
            }
        }
        while (write_pos < GRID_SIZE) {
            game.tiles[i][write_pos] = 0;
            write_pos++;
        }
        
        /* Merge */
        for (int j = 0; j < GRID_SIZE - 1; j++) {
            if (game.tiles[i][j] != 0 && game.tiles[i][j] == game.tiles[i][j + 1]) {
                game.tiles[i][j] *= 2;
                game.score += game.tiles[i][j];
                game.tiles[i][j + 1] = 0;
                game.moved = 1;
            }
        }
        
        /* Compact again */
        write_pos = 0;
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game.tiles[i][j] != 0) {
                game.tiles[i][write_pos] = game.tiles[i][j];
                write_pos++;
            }
        }
        while (write_pos < GRID_SIZE) {
            game.tiles[i][write_pos] = 0;
            write_pos++;
        }
    }
    
    /* Add new tile if moved */
    if (game.moved) {
        int empty_count = 0;
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                if (game.tiles[i][j] == 0) empty_count++;
            }
        }
        if (empty_count > 0) {
            int target = (game.score * 7 + empty_count * 11) % empty_count;
            int count = 0;
            for (int i = 0; i < GRID_SIZE; i++) {
                for (int j = 0; j < GRID_SIZE; j++) {
                    if (game.tiles[i][j] == 0) {
                        if (count == target) {
                            game.tiles[i][j] = 2;
                            return;
                        }
                        count++;
                    }
                }
            }
        }
    }
}

void game_2048_move_right(void) {
    /* Flip, move left, flip back */
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE / 2; j++) {
            int temp = game.tiles[i][j];
            game.tiles[i][j] = game.tiles[i][GRID_SIZE - 1 - j];
            game.tiles[i][GRID_SIZE - 1 - j] = temp;
        }
    }
    game_2048_move_left();
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE / 2; j++) {
            int temp = game.tiles[i][j];
            game.tiles[i][j] = game.tiles[i][GRID_SIZE - 1 - j];
            game.tiles[i][GRID_SIZE - 1 - j] = temp;
        }
    }
}

void game_2048_move_up(void) {
    /* Transpose, move left, transpose back */
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = i + 1; j < GRID_SIZE; j++) {
            int temp = game.tiles[i][j];
            game.tiles[i][j] = game.tiles[j][i];
            game.tiles[j][i] = temp;
        }
    }
    game_2048_move_left();
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = i + 1; j < GRID_SIZE; j++) {
            int temp = game.tiles[i][j];
            game.tiles[i][j] = game.tiles[j][i];
            game.tiles[j][i] = temp;
        }
    }
}

void game_2048_move_down(void) {
    /* Transpose, move right, transpose back */
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = i + 1; j < GRID_SIZE; j++) {
            int temp = game.tiles[i][j];
            game.tiles[i][j] = game.tiles[j][i];
            game.tiles[j][i] = temp;
        }
    }
    game_2048_move_right();
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = i + 1; j < GRID_SIZE; j++) {
            int temp = game.tiles[i][j];
            game.tiles[i][j] = game.tiles[j][i];
            game.tiles[j][i] = temp;
        }
    }
}

uint8_t get_tile_color(int value) {
    switch (value) {
        case 2:    return COLOR_BLUE;
        case 4:    return COLOR_CYAN;
        case 8:    return COLOR_GREEN;
        case 16:   return COLOR_LIGHT_GREEN;
        case 32:   return COLOR_YELLOW;
        case 64:   return COLOR_BROWN;
        case 128:  return COLOR_RED;
        case 256:  return COLOR_LIGHT_RED;
        case 512:  return COLOR_MAGENTA;
        case 1024: return COLOR_WHITE;
        default:   return COLOR_LIGHT_GRAY;
    }
}

void game_2048_draw(void) {
    int start_x = 40;
    int start_y = 90;
    
    if (needs_full_draw) {
        /* Full redraw - only once */
        vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
        
        /* Draw title */
        vga_putstring(100, 20, "2048 Game", COLOR_YELLOW, COLOR_BLACK);
        
        /* Draw score label */
        vga_putstring(20, 50, "Score: ", COLOR_WHITE, COLOR_BLACK);
        
        /* Draw grid background */
        vga_fillrect(start_x - 5, start_y - 5, GRID_SIZE * TILE_SIZE + 10, GRID_SIZE * TILE_SIZE + 10, COLOR_DARK_GRAY);
        
        /* Draw instructions */
        vga_putstring(20, SCREEN_HEIGHT - 50, "Arrows: Move | SPACE: Quit", COLOR_WHITE, COLOR_BLACK);
        
        needs_full_draw = 0;
    }
    
    /* Only draw tiles that changed */
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game.tiles[i][j] != game.old_tiles[i][j]) {
                int px = start_x + j * TILE_SIZE;
                int py = start_y + i * TILE_SIZE;
                
                if (game.tiles[i][j] == 0) {
                    vga_fillrect(px, py, TILE_SIZE - 2, TILE_SIZE - 2, COLOR_DARK_GRAY);
                } else {
                    uint8_t color = get_tile_color(game.tiles[i][j]);
                    vga_fillrect(px, py, TILE_SIZE - 2, TILE_SIZE - 2, color);
                    vga_rect(px, py, TILE_SIZE - 2, TILE_SIZE - 2, COLOR_BLACK);
                    /* Draw value (simplified) */
                    vga_putstring(px + 10, py + 12, "OK", COLOR_BLACK, color);
                }
                
                game.old_tiles[i][j] = game.tiles[i][j];
            }
        }
    }
    
    if (game_2048_is_game_over()) {
        vga_putstring(20, SCREEN_HEIGHT - 30, "GAME OVER!", COLOR_RED, COLOR_BLACK);
    }
}

void game_2048_run(void) {
    game_2048_init();
    
    while (!game_2048_is_game_over()) {
        game_2048_draw();
        
        if (keyboard_haskey()) {
            char key = keyboard_getchar();
            
            if (key == ' ') {
                return;
            } else if (key == (char)130) {  /* Left KEY_LEFT */
                game_2048_move_left();
            } else if (key == (char)131) {  /* Right KEY_RIGHT */
                game_2048_move_right();
            } else if (key == (char)128) {  /* Up KEY_UP */
                game_2048_move_up();
            } else if (key == (char)129) {  /* Down KEY_DOWN */
                game_2048_move_down();
            }
        }
        
        /* Frame delay */
        for (volatile int i = 0; i < 150000; i++);
    }
    
    game_2048_draw();
    
    /* Wait for keypress */
    while (!keyboard_haskey()) {
        for (volatile int i = 0; i < 150000; i++);
    }
}
