/*
 * terminal.c - Terminal/Shell with command execution
 */

#include "terminal.h"
#include "vga.h"
#include <stdint.h>

#define MAX_CMD_LEN 64
#define MAX_HISTORY 10
#define MAX_OUTPUT_LINES 100

static char cmd_buffer[MAX_CMD_LEN];
static int cmd_pos = 0;
static int last_cmd_pos = -1;  /* Track last cursor position */
static char output_lines[MAX_OUTPUT_LINES][MAX_CMD_LEN];
static int output_count = 0;
static int last_output_count = -1;  /* Track last output count */
static int scroll_offset = 0;  /* Lines scrolled up from bottom */

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

/* Simple in-memory filesystem */
#define MAX_FS_ENTRIES 32
#define MAX_FILENAME 32
#define MAX_FILECONTENT 256

typedef struct {
    char name[MAX_FILENAME];
    char content[MAX_FILECONTENT];
    int is_dir;
    int exists;
} fs_entry_t;

static fs_entry_t filesystem[MAX_FS_ENTRIES];
static char current_dir[MAX_CMD_LEN] = "/home/user";
static int fs_initialized = 0;

/* External password access */
extern char lock_password[32];

static void init_filesystem(void) {
    if (fs_initialized) return;
    
    /* Initialize with some default directories and files */
    for (int i = 0; i < MAX_FS_ENTRIES; i++) {
        filesystem[i].exists = 0;
    }
    
    /* Default directories */
    str_cpy(filesystem[0].name, "Desktop");
    filesystem[0].is_dir = 1;
    filesystem[0].exists = 1;
    
    str_cpy(filesystem[1].name, "Documents");
    filesystem[1].is_dir = 1;
    filesystem[1].exists = 1;
    
    str_cpy(filesystem[2].name, "Downloads");
    filesystem[2].is_dir = 1;
    filesystem[2].exists = 1;
    
    /* Default files */
    str_cpy(filesystem[3].name, "readme.txt");
    str_cpy(filesystem[3].content, "Welcome to GegOS!");
    filesystem[3].is_dir = 0;
    filesystem[3].exists = 1;
    
    str_cpy(filesystem[4].name, "hello.txt");
    str_cpy(filesystem[4].content, "Hello, World!");
    filesystem[4].is_dir = 0;
    filesystem[4].exists = 1;
    
    fs_initialized = 1;
}

/* Command execution */
static void exec_help(void) {
    add_output("Available commands:");
    add_output("  help       - Show this help");
    add_output("  clear      - Clear screen");
    add_output("  ls         - List files");
    add_output("  cd DIR     - Change directory");
    add_output("  pwd        - Print working directory");
    add_output("  mkdir DIR  - Create directory");
    add_output("  touch FILE - Create empty file");
    add_output("  nano FILE  - Edit file (simulated)");
    add_output("  cat FILE   - Show file contents");
    add_output("  passwd     - Change lock password");
    add_output("  uname      - System information");
    add_output("  echo TEXT  - Print text");
}

static void exec_clear(void) {
    output_count = 0;
    scroll_offset = 0;
}

static void exec_ls(void) {
    init_filesystem();
    for (int i = 0; i < MAX_FS_ENTRIES; i++) {
        if (filesystem[i].exists) {
            char line[MAX_CMD_LEN];
            str_cpy(line, filesystem[i].name);
            if (filesystem[i].is_dir) {
                int len = str_len(line);
                line[len] = '/';
                line[len+1] = 0;
            }
            add_output(line);
        }
    }
}

static void exec_pwd(void) {
    add_output(current_dir);
}

static void exec_cd(const char* args) {
    if (str_len(args) <= 3) {
        str_cpy(current_dir, "/home/user");
        add_output("Changed to /home/user");
        return;
    }
    const char* dir = args + 3;
    if (dir[0] == '.' && dir[1] == '.') {
        str_cpy(current_dir, "/home");
        add_output("Changed to /home");
    } else if (dir[0] == '/') {
        str_cpy(current_dir, dir);
        add_output("Changed directory");
    } else {
        /* Append to current dir */
        int len = str_len(current_dir);
        current_dir[len] = '/';
        str_cpy(current_dir + len + 1, dir);
        add_output("Changed directory");
    }
}

static void exec_mkdir(const char* args) {
    init_filesystem();
    if (str_len(args) <= 6) {
        add_output("Usage: mkdir <dirname>");
        return;
    }
    const char* dirname = args + 6;
    /* Find free slot */
    for (int i = 0; i < MAX_FS_ENTRIES; i++) {
        if (!filesystem[i].exists) {
            str_cpy(filesystem[i].name, dirname);
            filesystem[i].is_dir = 1;
            filesystem[i].exists = 1;
            add_output("Directory created");
            return;
        }
    }
    add_output("Error: No space for new directory");
}

static void exec_touch(const char* args) {
    init_filesystem();
    if (str_len(args) <= 6) {
        add_output("Usage: touch <filename>");
        return;
    }
    const char* filename = args + 6;
    /* Check if exists */
    for (int i = 0; i < MAX_FS_ENTRIES; i++) {
        if (filesystem[i].exists) {
            int match = 1;
            int j = 0;
            while (filesystem[i].name[j] || filename[j]) {
                if (filesystem[i].name[j] != filename[j]) {
                    match = 0;
                    break;
                }
                j++;
            }
            if (match) {
                add_output("File already exists");
                return;
            }
        }
    }
    /* Find free slot */
    for (int i = 0; i < MAX_FS_ENTRIES; i++) {
        if (!filesystem[i].exists) {
            str_cpy(filesystem[i].name, filename);
            filesystem[i].content[0] = 0;
            filesystem[i].is_dir = 0;
            filesystem[i].exists = 1;
            add_output("File created");
            return;
        }
    }
    add_output("Error: No space for new file");
}

static void exec_cat(const char* args) {
    init_filesystem();
    if (str_len(args) <= 4) {
        add_output("Usage: cat <filename>");
        return;
    }
    const char* filename = args + 4;
    for (int i = 0; i < MAX_FS_ENTRIES; i++) {
        if (filesystem[i].exists && !filesystem[i].is_dir) {
            int match = 1;
            int j = 0;
            while (filesystem[i].name[j] || filename[j]) {
                if (filesystem[i].name[j] != filename[j]) {
                    match = 0;
                    break;
                }
                j++;
            }
            if (match) {
                if (filesystem[i].content[0]) {
                    add_output(filesystem[i].content);
                } else {
                    add_output("(empty file)");
                }
                return;
            }
        }
    }
    add_output("File not found");
}

static void exec_nano(const char* args) {
    if (str_len(args) <= 5) {
        add_output("Usage: nano <filename>");
        return;
    }
    add_output("nano: Text editor not available");
    add_output("Use touch to create files");
}

static void exec_passwd(const char* args) {
    if (str_len(args) <= 7) {
        add_output("Usage: passwd <newpassword>");
        add_output("Current password is 'gegos'");
        return;
    }
    const char* newpass = args + 7;
    str_cpy(lock_password, newpass);
    add_output("Password changed successfully");
}

static void exec_uname(void) {
    add_output("GegOS 1.0.0 x86 i686");
    add_output("Kernel: GegOS-32bit");
    add_output("Built: Feb 2026");
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
    add_output("GegOS [Version 1.0]");
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
    } else if (str_startswith(cmd, "cd")) {
        exec_cd(cmd);
    } else if (str_startswith(cmd, "mkdir ")) {
        exec_mkdir(cmd);
    } else if (str_startswith(cmd, "touch ")) {
        exec_touch(cmd);
    } else if (str_startswith(cmd, "cat ")) {
        exec_cat(cmd);
    } else if (str_startswith(cmd, "nano ")) {
        exec_nano(cmd);
    } else if (str_startswith(cmd, "passwd")) {
        exec_passwd(cmd);
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
        add_output("-bash: command not found");
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
        scroll_offset = 0;  /* Reset scroll on new command */
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

void terminal_scroll_up(void) {
    scroll_offset += 3;  /* Scroll up 3 lines */
}

void terminal_scroll_down(void) {
    scroll_offset -= 3;  /* Scroll down 3 lines */
    if (scroll_offset < 0) scroll_offset = 0;
}

void terminal_draw(int x, int y, int width, int height) {
    int line_height = 12;
    (void)y;  /* Suppress unused warning */
    int max_lines = (height - 30) / line_height;
    
    /* Calculate visible range with scroll offset */
    int end_line = output_count - scroll_offset;
    int start_line = (end_line > max_lines) ? (end_line - max_lines) : 0;
    
    /* Clamp scroll offset */
    if (scroll_offset < 0) scroll_offset = 0;
    if (scroll_offset > output_count - max_lines && output_count > max_lines) {
        scroll_offset = output_count - max_lines;
    }
    
    /* Force redraw if scroll changed or output changed */
    static int last_scroll_offset = -1;
    if (last_output_count != output_count || last_scroll_offset != scroll_offset) {
        /* Gray background */
        vga_fillrect(x, y, width, height, COLOR_LIGHT_GRAY);
        
        /* Black terminal area (sunken) */
        int term_x = x + 3;
        int term_y = y + 3;
        int term_w = width - 6;
        int term_h_inner = height - 6;

        vga_fillrect(term_x, term_y, term_w, term_h_inner, COLOR_BLACK);
        /* Sunken border */
        vga_hline(term_x, term_y, term_w, COLOR_DARK_GRAY);
        vga_vline(term_x, term_y, term_h_inner, COLOR_DARK_GRAY);
        vga_hline(term_x, term_y + term_h_inner - 1, term_w, COLOR_WHITE);
        vga_vline(term_x + term_w - 1, term_y, term_h_inner, COLOR_WHITE);
        
        /* Draw output lines */
        for (int i = start_line; i < end_line && i < output_count; i++) {
            int line_y = term_y + 5 + (i - start_line) * line_height;
            vga_putstring(term_x + 5, line_y, output_lines[i], COLOR_WHITE, COLOR_BLACK);
        }
        last_output_count = output_count;
        last_scroll_offset = scroll_offset;
    }
    
    /* Draw prompt line only if at bottom */
    int term_x = x + 3;
    int term_y = y + 3;
    (void)height;  /* Used in calculations above */
    
    if (scroll_offset == 0) {
        int visible_lines = (end_line - start_line);
        int prompt_y = term_y + 5 + visible_lines * line_height;
        
        if (prompt_y < y + height - 15) {
            /* Only redraw prompt line if cursor position changed */
            if (last_cmd_pos != cmd_pos) {
                /* Clear only the prompt line */
                vga_fillrect(term_x, prompt_y, width - 6, line_height, COLOR_BLACK);
                
                vga_putstring(term_x + 5, prompt_y, "$ ", COLOR_LIGHT_GREEN, COLOR_BLACK);
                vga_putstring(term_x + 20, prompt_y, cmd_buffer, COLOR_WHITE, COLOR_BLACK);
                
                /* Cursor */
                int cursor_x = term_x + 20 + (cmd_pos * 8);
                vga_fillrect(cursor_x, prompt_y, 8, 10, COLOR_WHITE);
                
                last_cmd_pos = cmd_pos;
            }
        }
    }
}
