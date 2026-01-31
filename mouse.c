/*
 * mouse.c - PS/2 Mouse Driver for GegOS
 * Polling-based mouse input
 */

#include "mouse.h"
#include "io.h"
#include "vga.h"

/* PS/2 Ports */
#define MOUSE_DATA_PORT   0x60
#define MOUSE_STATUS_PORT 0x64
#define MOUSE_CMD_PORT    0x64

/* PS/2 Commands */
#define MOUSE_CMD_WRITE   0xD4
#define MOUSE_ENABLE_AUX  0xA8
#define MOUSE_GET_COMPAQ  0x20
#define MOUSE_SET_COMPAQ  0x60

/* Mouse commands */
#define MOUSE_SET_DEFAULTS   0xF6
#define MOUSE_ENABLE_PACKET  0xF4
#define MOUSE_SET_SAMPLE     0xF3

/* Mouse state */
static mouse_state_t mouse_state;
static int mouse_cycle = 0;
static int8_t mouse_bytes[3];
static int min_x = 0, min_y = 0;
static int max_x = SCREEN_WIDTH - 1;
static int max_y = SCREEN_HEIGHT - 1;

/* Wait for mouse controller to be ready for input */
static void mouse_wait_write(void) {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(MOUSE_STATUS_PORT) & 0x02) == 0) return;
    }
}

/* Wait for mouse data to be available */
static void mouse_wait_read(void) {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(MOUSE_STATUS_PORT) & 0x01) != 0) return;
    }
}

/* Send command to mouse */
static void mouse_write(uint8_t data) {
    mouse_wait_write();
    outb(MOUSE_CMD_PORT, MOUSE_CMD_WRITE);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, data);
}

/* Read from mouse */
static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(MOUSE_DATA_PORT);
}

/* Initialize mouse */
void mouse_init(void) {
    uint8_t status;
    
    /* Initialize state */
    mouse_state.x = SCREEN_WIDTH / 2;
    mouse_state.y = SCREEN_HEIGHT / 2;
    mouse_state.dx = 0;
    mouse_state.dy = 0;
    mouse_state.buttons = 0;
    mouse_state.prev_buttons = 0;
    mouse_cycle = 0;
    
    /* Enable auxiliary mouse device */
    mouse_wait_write();
    outb(MOUSE_CMD_PORT, MOUSE_ENABLE_AUX);
    
    /* Enable interrupts (get compaq status byte) */
    mouse_wait_write();
    outb(MOUSE_CMD_PORT, MOUSE_GET_COMPAQ);
    mouse_wait_read();
    status = inb(MOUSE_DATA_PORT);
    
    /* Set compaq status byte (enable IRQ12) */
    status |= 0x02;   /* Enable IRQ12 */
    status &= ~0x20;  /* Enable mouse clock */
    
    mouse_wait_write();
    outb(MOUSE_CMD_PORT, MOUSE_SET_COMPAQ);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, status);
    
    /* Set mouse defaults */
    mouse_write(MOUSE_SET_DEFAULTS);
    mouse_read();  /* ACK */
    
    /* Set sample rate to 100 */
    mouse_write(MOUSE_SET_SAMPLE);
    mouse_read();  /* ACK */
    mouse_write(100);
    mouse_read();  /* ACK */
    
    /* Enable mouse packet streaming */
    mouse_write(MOUSE_ENABLE_PACKET);
    mouse_read();  /* ACK */
    
    /* Flush buffer */
    while (inb(MOUSE_STATUS_PORT) & 0x01) {
        inb(MOUSE_DATA_PORT);
    }
}

/* Update mouse state */
void mouse_update(void) {
    /* Check if data available */
    uint8_t status = inb(MOUSE_STATUS_PORT);
    
    if (!(status & 0x01)) return;  /* No data */
    if (!(status & 0x20)) return;  /* Not from mouse */
    
    uint8_t data = inb(MOUSE_DATA_PORT);
    
    switch (mouse_cycle) {
        case 0:
            /* First byte - buttons and overflow */
            if (data & 0x08) {  /* Always-1 bit should be set */
                mouse_bytes[0] = data;
                mouse_cycle = 1;
            }
            break;
            
        case 1:
            /* Second byte - X movement */
            mouse_bytes[1] = data;
            mouse_cycle = 2;
            break;
            
        case 2:
            /* Third byte - Y movement */
            mouse_bytes[2] = data;
            mouse_cycle = 0;
            
            /* Save previous buttons */
            mouse_state.prev_buttons = mouse_state.buttons;
            
            /* Update buttons */
            mouse_state.buttons = mouse_bytes[0] & 0x07;
            
            /* Calculate movement (sign-extend if needed) */
            int dx = mouse_bytes[1];
            int dy = mouse_bytes[2];
            
            if (mouse_bytes[0] & 0x10) dx |= 0xFFFFFF00;  /* X sign bit */
            if (mouse_bytes[0] & 0x20) dy |= 0xFFFFFF00;  /* Y sign bit */
            
            /* Check for overflow */
            if (mouse_bytes[0] & 0x40) dx = 0;  /* X overflow */
            if (mouse_bytes[0] & 0x80) dy = 0;  /* Y overflow */
            
            mouse_state.dx = dx;
            mouse_state.dy = -dy;  /* Y is inverted */
            
            /* Update position with bounds checking */
            mouse_state.x += dx;
            mouse_state.y -= dy;  /* Y inverted */
            
            if (mouse_state.x < min_x) mouse_state.x = min_x;
            if (mouse_state.x > max_x) mouse_state.x = max_x;
            if (mouse_state.y < min_y) mouse_state.y = min_y;
            if (mouse_state.y > max_y) mouse_state.y = max_y;
            break;
    }
}

/* Get mouse state */
mouse_state_t* mouse_get_state(void) {
    return &mouse_state;
}

/* Get X position */
int mouse_get_x(void) {
    return mouse_state.x;
}

/* Get Y position */
int mouse_get_y(void) {
    return mouse_state.y;
}

/* Button down */
int mouse_button_down(uint8_t button) {
    return (mouse_state.buttons & button) != 0;
}

/* Button clicked (just pressed) */
int mouse_button_clicked(uint8_t button) {
    return (mouse_state.buttons & button) && !(mouse_state.prev_buttons & button);
}

/* Button released */
int mouse_button_released(uint8_t button) {
    return !(mouse_state.buttons & button) && (mouse_state.prev_buttons & button);
}

/* Set position */
void mouse_set_position(int x, int y) {
    mouse_state.x = x;
    mouse_state.y = y;
}

/* Set bounds */
void mouse_set_bounds(int minx, int miny, int maxx, int maxy) {
    min_x = minx;
    min_y = miny;
    max_x = maxx;
    max_y = maxy;
}
