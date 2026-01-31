/*
 * pong.c - Simple Pong Game for GegOS
 */

#include "vga.h"
#include "gui.h"
#include "mouse.h"
#include "io.h"

#define PADDLE_WIDTH   8
#define PADDLE_HEIGHT  40
#define BALL_SIZE      4
#define PONG_WIDTH     320
#define PONG_HEIGHT    180

typedef struct {
    int x, y;
    int vx, vy;
} ball_t;

typedef struct {
    int y;
} paddle_t;

static ball_t ball;
static paddle_t left_paddle;
static paddle_t right_paddle;
static int left_score = 0;
static int right_score = 0;
static int pong_running = 0;

void pong_init(void) {
    ball.x = PONG_WIDTH / 2;
    ball.y = PONG_HEIGHT / 2;
    ball.vx = 2;
    ball.vy = 1;
    
    left_paddle.y = PONG_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    right_paddle.y = PONG_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    left_score = 0;
    right_score = 0;
    pong_running = 1;
}

void pong_update(void) {
    if (!pong_running) return;
    
    /* Update ball position */
    ball.x += ball.vx;
    ball.y += ball.vy;
    
    /* Bounce off top and bottom */
    if (ball.y < 0 || ball.y + BALL_SIZE > PONG_HEIGHT) {
        ball.vy = -ball.vy;
        if (ball.y < 0) ball.y = 0;
        if (ball.y > PONG_HEIGHT - BALL_SIZE) ball.y = PONG_HEIGHT - BALL_SIZE;
    }
    
    /* AI: move right paddle to track ball */
    int paddle_center = right_paddle.y + PADDLE_HEIGHT / 2;
    if (paddle_center < ball.y - 10) {
        right_paddle.y += 2;
    } else if (paddle_center > ball.y + 10) {
        right_paddle.y -= 2;
    }
    
    if (right_paddle.y < 0) right_paddle.y = 0;
    if (right_paddle.y + PADDLE_HEIGHT > PONG_HEIGHT) 
        right_paddle.y = PONG_HEIGHT - PADDLE_HEIGHT;
    
    /* Ball collision with left paddle */
    if (ball.x - BALL_SIZE <= 10 && ball.x + BALL_SIZE >= 10 - PADDLE_WIDTH &&
        ball.y >= left_paddle.y && ball.y <= left_paddle.y + PADDLE_HEIGHT) {
        ball.vx = -ball.vx;
        ball.x = 10;
    }
    
    /* Ball collision with right paddle */
    if (ball.x + BALL_SIZE >= PONG_WIDTH - 10 && ball.x - BALL_SIZE <= PONG_WIDTH - 10 + PADDLE_WIDTH &&
        ball.y >= right_paddle.y && ball.y <= right_paddle.y + PADDLE_HEIGHT) {
        ball.vx = -ball.vx;
        ball.x = PONG_WIDTH - 10 - BALL_SIZE;
    }
    
    /* Ball out of bounds - score */
    if (ball.x < 0) {
        right_score++;
        pong_init();
    }
    if (ball.x > PONG_WIDTH) {
        left_score++;
        pong_init();
    }
}

void pong_draw(void) {
    /* Clear play area */
    vga_fillrect(0, 0, PONG_WIDTH, PONG_HEIGHT, COLOR_BLACK);
    
    /* Center line */
    for (int i = 0; i < PONG_HEIGHT; i += 10) {
        vga_fillrect(PONG_WIDTH / 2 - 1, i, 2, 5, COLOR_WHITE);
    }
    
    /* Draw paddles */
    vga_fillrect(5, left_paddle.y, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_WHITE);
    vga_fillrect(PONG_WIDTH - 5 - PADDLE_WIDTH, right_paddle.y, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_WHITE);
    
    /* Draw ball */
    vga_fillrect(ball.x, ball.y, BALL_SIZE, BALL_SIZE, COLOR_WHITE);
    
    /* Draw scores */
    int score_y = PONG_HEIGHT - 15;
    
    (void)score_y;  /* Suppress unused warning */
}

void pong_handle_mouse(int x, int y) {
    (void)x;  /* Suppress unused parameter warning */
    /* Control left paddle with mouse */
    if (y > 0 && y < PONG_HEIGHT - PADDLE_HEIGHT) {
        left_paddle.y = y;
    }
}

void pong_run(void) {
    pong_init();
    int frame_count = 0;
    
    while (pong_running && frame_count < 300) {
        mouse_update();
        int mx = mouse_get_x();
        int my = mouse_get_y();
        
        pong_update();
        pong_draw();
        pong_handle_mouse(mx, my);
        
        frame_count++;
        
        /* Limit game speed */
        for (volatile int i = 0; i < 10000; i++);
    }
}
