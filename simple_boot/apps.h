/*
 * apps.h - Application System for GegOS
 * Supports running different file types
 */

#ifndef APPS_H
#define APPS_H

#include <stdint.h>

/* File types */
typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_GEG,      /* Native GegOS app */
    FILE_TYPE_EXE,      /* Executable (simulated) */
    FILE_TYPE_BAT,      /* Batch script */
    FILE_TYPE_VBS,      /* VBScript (simulated) */
    FILE_TYPE_TXT,      /* Text file */
} file_type_t;

/* Application structure */
typedef struct {
    const char* name;
    const char* icon;
    void (*run)(void);
    int running;
} app_t;

/* File entry for file manager */
typedef struct {
    const char* name;
    file_type_t type;
    const char* content;
    int size;
} file_entry_t;

/* Initialize app system */
void apps_init(void);

/* Run an application by name */
int app_run(const char* name);

/* Get file type from extension */
file_type_t get_file_type(const char* filename);

/* Execute a file */
int file_execute(const char* filename);

/* Built-in apps */
void app_browser(void);
void app_notepad(void);
void app_terminal(void);
void app_files(void);
void app_about(void);
void app_calculator(void);
void app_settings(void);

/* Get app count */
int apps_get_count(void);

/* Get app by index */
app_t* apps_get(int index);

#endif /* APPS_H */
