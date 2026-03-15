#ifndef LAUNCHER_BROWSER_INTERNAL_H
#define LAUNCHER_BROWSER_INTERNAL_H

#include "launcher_browser.h"
#include <dirent.h>
#include <stddef.h>

#define LAUNCHER_BROWSER_ROOT ""
#define LAUNCHER_BROWSER_ROOT_LABEL "DEVICES"
#define LAUNCHER_BROWSER_LOAD_CHUNK 64
#define LAUNCHER_BROWSER_CAPACITY_GROW 128
#define LAUNCHER_BROWSER_MAX_USB_DEVICES 10

extern launcher_browser_entry_t *g_entries;
extern int g_entry_count;
extern int g_entry_capacity;
extern char g_current_path[256];
extern int g_selected;
extern int g_scroll;
extern int g_last_error;
extern DIR *g_scan_dir;
extern int g_scan_done;

int launcher_browser_has_rom_ext(const char *name);
int launcher_browser_is_root_path(const char *path);
void launcher_browser_close_scan_dir(void);
void launcher_browser_clear_entries(void);
int launcher_browser_ensure_capacity(int need);
int launcher_browser_append_entry(const char *name, int is_dir);

int launcher_browser_scan_root_devices(void);
int launcher_browser_open_scan_dir(const char *path);
void launcher_browser_path_join(char *out, size_t out_size, const char *base, const char *name);
int launcher_browser_load_more_entries(int want);

#endif
