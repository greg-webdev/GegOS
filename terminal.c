/*
 * terminal.c - Terminal/Shell with command execution
 */

#include "terminal.h"
#include "vga.h"
#include <stdint.h>

#define MAX_CMD_LEN 64
#define MAX_HISTORY 10
#define MAX_OUTPUT_LINES 20

static char cmd_buffer[MAX_CMD_LEN];
static int cmd_pos = 0;
static char output_lines[MAX_OUTPUT_LINES][MAX_CMD_LEN];
static int output_count = 0;
static int scroll_offset = 0;

/* String utilities */
static int str_len(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

static int str_cmp(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] && s2[i] && s1[i] == s2[i]) i++;
    return s1[i] - s2[i];
}

static void str_cpy(char* dst, const char* src) {
    int i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static int str_startswith(const char* str, const char* prefix) {
    int i = 0;
    while (prefix[i]) {
        if (str[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

/* Output helpers */
static void add_output(const char* line) {
    if (output_count >= MAX_OUTPUT_LINES) {
        /* Scroll up */
        for (int i = 0; i < MAX_OUTPUT_LINES - 1; i++) {
            str_cpy(output_lines[i], output_lines[i + 1]);
        }
        output_count = MAX_OUTPUT_LINES - 1;
    }
    str_cpy(output_lines[output_count], line);
    output_count++;
}

/* Command execution */
static void exec_help(void) {
    add_output("Available commands:");
    add_output("  help       - Show this help");
    add_output("  clear      - Clear screen");
    add_output("  ls         - List files");
    add_output("  pwd        - Print working directory");
    add_output("  uname      - System information");
    add_output("  apt list   - List installed packages");
    add_output("  apt update - Update package lists");
    add_output("  dir        - Directory listing (Windows)");
    add_output("  ver        - Version info (Windows)");
    add_output("  echo TEXT  - Print text");
}

static void exec_clear(void) {
    output_count = 0;
    scroll_offset = 0;
}

static void exec_ls(void) {
    add_output("Desktop/");
    add_output("Documents/");
    add_output("Downloads/");
    add_output("System/");
    add_output("bin/");
    add_output("etc/");
}

static void exec_pwd(void) {
    add_output("/home/user");
}

static void exec_uname(void) {
    add_output("GegOS 1.0.0 x86 i686");
    add_output("Kernel: GegOS-32bit");
    add_output("Built: Jan 2026");
}

static void exec_apt_list(void) {
    add_output("Installed packages:");
    add_output("  bash          5.1-6");
    add_output("  coreutils     9.1-1");
    add_output("  gcc           12.2.0-1");
    add_output("  python3       3.11.2-1");
    add_output("  vim           9.0-1");
    add_output("  curl          7.88.1-1");
    add_output("  git           2.39.2-1");
}

static void exec_apt_update(void) {
    add_output("Hit:1 http://geg.os/repo stable InRelease");
    add_output("Get:2 http://geg.os/repo stable/main i686");
    add_output("Fetched 1,234 kB in 0s (4,321 kB/s)");
    add_output("Reading package lists... Done");
}

static void exec_dir(void) {
    add_output(" Directory of C:\\Users\\User");
    add_output("");
    add_output("01/31/2026  10:30 AM    <DIR>          Desktop");
    add_output("01/31/2026  10:30 AM    <DIR>          Documents");
    add_output("01/31/2026  10:30 AM    <DIR>          Downloads");
    add_output("               0 File(s)              0 bytes");
}

static void exec_ver(void) {
    add_output("GegOS [Version 1.0.0.32]");
    add_output("(c) 2026 GegOS Corporation.");
}

static void exec_echo(const char* args) {
    if (str_len(args) > 5) {
        add_output(args + 5);
    } else {
        add_output("");
    }
}

static void exec_command(const char* cmd) {
    if (str_len(cmd) == 0) return;
    
    /* Show command */
    char prompt[MAX_CMD_LEN];
    prompt[0] = '$';
    prompt[1] = ' ';
    str_cpy(prompt + 2, cmd);
    add_output(prompt);
    
    /* Execute */
    if (str_cmp(cmd, "help") == 0) {
        exec_help();
    } else if (str_cmp(cmd, "clear") == 0) {
        exec_clear();
    } else if (str_cmp(cmd, "ls") == 0) {
        exec_ls();
    } else if (str_cmp(cmd, "pwd") == 0) {
        exec_pwd();
    } else if (str_cmp(cmd, "uname") == 0) {
        exec_uname();
    } else if (str_cmp(cmd, "apt list") == 0) {
        exec_apt_list();
    } else if (str_cmp(cmd, "apt update") == 0) {
        exec_apt_update();
    } else if (str_cmp(cmd, "dir") == 0) {
        exec_dir();
    } else if (str_cmp(cmd, "ver") == 0) {
        exec_ver();
    } else if (str_startswith(cmd, "echo ")) {
        exec_echo(cmd);
    } else {
        char error[MAX_CMD_LEN];
        error[0] = '-';
        error[1] = 'b';
        error[2] = 'a';
        error[3] = 's';
        error[4] = 'h';
        error[5] = ':';
        error[6] = ' ';
        int i = 0;
        while (cmd[i] && i < 20) {
            error[7 + i] = cmd[i];
            i++;
        }
        error[7 + i] = ':';
        error[8 + i] = ' ';
        error[9 + i] = 'c';
        error[10 + i] = 'm';
        error[11 + i] = 'd';
        error[12 + i] = ' ';
        error[13 + i] = 'n';
        error[14 + i] = 'o';
        error[15 + i] = 't';
        error[16 + i] = ' ';
        error[17 + i] = 'f';
        error[18 + i] = 'o';
        error[19 + i] = 'u';
        error[20 + i] = 'n';
        error[21 + i] = 'd';
        error[22 + i] = 0;
        add_output(error);
    }
}

void terminal_init(void) {
    cmd_pos = 0;
    output_count = 0;
    cmd_buffer[0] = 0;
    
    add_output("GegOS Terminal v1.0");
    add_output("Type 'help' for commands");
    add_output("");
}

void terminal_handle_key(char c) {
    if (c == '\n') {
        /* Execute command */
        cmd_buffer[cmd_pos] = 0;
        exec_command(cmd_buffer);
        cmd_pos = 0;
        cmd_buffer[0] = 0;
    } else if (c == '\b') {
        /* Backspace */
        if (cmd_pos > 0) {
            cmd_pos--;
            cmd_buffer[cmd_pos] = 0;
        }
    } else if (c >= 32 && c < 127) {
        /* Normal character */
        if (cmd_pos < MAX_CMD_LEN - 1) {
            cmd_buffer[cmd_pos] = c;
            cmd_pos++;
            cmd_buffer[cmd_pos] = 0;
        }
    }
}

void terminal_draw(int x, int y, int width, int height) {
    /* Background */
    vga_fillrect(x, y, width, height, COLOR_BLACK);
    
    /* Draw output lines */
    int line_height = 12;
    int start_y = y + 5;
    int max_lines = (height - 30) / line_height;
    
    int start_line = (output_count > max_lines) ? (output_count - max_lines) : 0;
    
    for (int i = start_line; i < output_count; i++) {
        int line_y = start_y + (i - start_line) * line_height;
        vga_putstring(x + 5, line_y, output_lines[i], COLOR_WHITE, COLOR_BLACK);
    }
    
    /* Draw prompt */
    int prompt_y = start_y + (output_count - start_line) * line_height;
    if (prompt_y < y + height - 15) {
        vga_putstring(x + 5, prompt_y, "$ ", COLOR_LIGHT_GREEN, COLOR_BLACK);
        vga_putstring(x + 20, prompt_y, cmd_buffer, COLOR_WHITE, COLOR_BLACK);
        
        /* Cursor */
        int cursor_x = x + 20 + (cmd_pos * 8);
        vga_fillrect(cursor_x, prompt_y, 8, 10, COLOR_WHITE);
    }
}
