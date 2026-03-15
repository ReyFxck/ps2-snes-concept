#include "launcher_browser.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define LAUNCHER_BROWSER_ROOT ""
#define LAUNCHER_BROWSER_ROOT_LABEL "DEVICES"
#define LAUNCHER_BROWSER_LOAD_CHUNK 64
#define LAUNCHER_BROWSER_CAPACITY_GROW 128
#define LAUNCHER_BROWSER_MAX_USB_DEVICES 10

static launcher_browser_entry_t *g_entries = NULL;
static int g_entry_count = 0;
static int g_entry_capacity = 0;

static char g_current_path[256];
static int g_selected = 0;
static int g_scroll = 0;
static int g_last_error = 0;

static DIR *g_scan_dir = NULL;
static int g_scan_done = 1;

static int has_rom_ext(const char *name)
{
    const char *dot = strrchr(name, '.');

    if (!dot)
        return 0;

    if (!strcmp(dot, ".smc") || !strcmp(dot, ".SMC"))
        return 1;
    if (!strcmp(dot, ".sfc") || !strcmp(dot, ".SFC"))
        return 1;
    if (!strcmp(dot, ".swc") || !strcmp(dot, ".SWC"))
        return 1;
    if (!strcmp(dot, ".fig") || !strcmp(dot, ".FIG"))
        return 1;
    if (!strcmp(dot, ".zip") || !strcmp(dot, ".ZIP"))
        return 1;

    return 0;
}

static int is_root_path(const char *path)
{
    return !path || !path[0];
}

static void close_scan_dir(void)
{
    if (g_scan_dir) {
        closedir(g_scan_dir);
        g_scan_dir = NULL;
    }
}

static void clear_entries(void)
{
    g_entry_count = 0;
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
}

static int ensure_capacity(int need)
{
    launcher_browser_entry_t *tmp;
    int new_capacity;

    if (need <= g_entry_capacity)
        return 1;

    new_capacity = g_entry_capacity;
    if (new_capacity <= 0)
        new_capacity = LAUNCHER_BROWSER_CAPACITY_GROW;

    while (new_capacity < need)
        new_capacity += LAUNCHER_BROWSER_CAPACITY_GROW;

    tmp = (launcher_browser_entry_t *)realloc(g_entries, sizeof(*g_entries) * new_capacity);
    if (!tmp) {
        g_last_error = 1;
        return 0;
    }

    g_entries = tmp;
    g_entry_capacity = new_capacity;
    return 1;
}

static int append_entry(const char *name, int is_dir)
{
    if (!ensure_capacity(g_entry_count + 1))
        return 0;

    snprintf(g_entries[g_entry_count].name, sizeof(g_entries[g_entry_count].name), "%s", name);
    g_entries[g_entry_count].is_dir = is_dir;
    g_entry_count++;
    return 1;
}

static int device_exists(const char *path)
{
    DIR *d;
    struct stat st;

    d = opendir(path);
    if (d) {
        closedir(d);
        return 1;
    }

    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return 1;

    return 0;
}

static int scan_root_devices(void)
{
    char probe[8] = "mass0:/";
    int found = 0;
    int i;

    clear_entries();
    close_scan_dir();
    g_scan_done = 1;

    for (i = 0; i < LAUNCHER_BROWSER_MAX_USB_DEVICES; i++) {
        probe[4] = '0' + i;

        if (!device_exists(probe))
            continue;

        if (i == 0) {
            if (!append_entry("mass:/", 1))
                return 0;
        } else {
            char label[8];
            snprintf(label, sizeof(label), "mass%d:/", i);
            if (!append_entry(label, 1))
                return 0;
        }

        found = 1;
    }

    if (!found)
        g_last_error = 1;

    return found;
}

static int open_scan_dir(const char *path)
{
    close_scan_dir();
    g_scan_dir = opendir(path);
    if (!g_scan_dir) {
        g_last_error = 1;
        g_scan_done = 1;
        return 0;
    }

    g_scan_done = 0;
    return 1;
}

static void path_join(char *out, size_t out_size, const char *base, const char *name)
{
    size_t len = strlen(base);

    if (len > 0 && base[len - 1] == '/')
        snprintf(out, out_size, "%s%s", base, name);
    else
        snprintf(out, out_size, "%s/%s", base, name);
}

static int load_more_entries(int want)
{
    int added = 0;

    if (g_scan_done)
        return 1;

    while (added < want) {
        struct dirent *de;
        char full[512];
        struct stat st;
        int is_dir = 0;
        int keep = 0;

        de = readdir(g_scan_dir);
        if (!de) {
            close_scan_dir();
            g_scan_done = 1;
            break;
        }

        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        path_join(full, sizeof(full), g_current_path, de->d_name);

        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            keep = 1;
            is_dir = 1;
        } else if (has_rom_ext(de->d_name)) {
            keep = 1;
            is_dir = 0;
        }

        if (!keep)
            continue;

        if (!append_entry(de->d_name, is_dir))
            return 0;

        added++;
    }

    return 1;
}

static int reset_to_path(const char *path)
{
    snprintf(g_current_path, sizeof(g_current_path), "%s", path ? path : "");
    clear_entries();

    if (is_root_path(g_current_path))
        return scan_root_devices();

    if (!open_scan_dir(g_current_path))
        return 0;

    return load_more_entries(LAUNCHER_BROWSER_LOAD_CHUNK);
}

void launcher_browser_init(void)
{
    g_entries = NULL;
    g_entry_count = 0;
    g_entry_capacity = 0;
    g_current_path[0] = '\0';
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
    g_scan_dir = NULL;
    g_scan_done = 1;
}

int launcher_browser_open(const char *path)
{
    if (!path || !path[0])
        return reset_to_path(LAUNCHER_BROWSER_ROOT);

    return reset_to_path(path);
}

int launcher_browser_refresh(void)
{
    return reset_to_path(g_current_path);
}

int launcher_browser_go_parent(void)
{
    char temp[256];
    char *colon;
    char *slash;

    if (is_root_path(g_current_path))
        return 0;

    snprintf(temp, sizeof(temp), "%s", g_current_path);

    if (strlen(temp) > 0 && temp[strlen(temp) - 1] == '/')
        temp[strlen(temp) - 1] = '\0';

    colon = strchr(temp, ':');
    slash = strrchr(temp, '/');

    if (!slash || !colon || slash <= (colon + 1))
        temp[0] = '\0';
    else
        *slash = '\0';

    return launcher_browser_open(temp);
}

int launcher_browser_count(void)
{
    return g_entry_count;
}

int launcher_browser_selected(void)
{
    return g_selected;
}

int launcher_browser_scroll(void)
{
    return g_scroll;
}

const char *launcher_browser_current_path(void)
{
    if (is_root_path(g_current_path))
        return LAUNCHER_BROWSER_ROOT_LABEL;

    return g_current_path;
}

const launcher_browser_entry_t *launcher_browser_entry(int index)
{
    if (index < 0 || index >= g_entry_count)
        return NULL;

    return &g_entries[index];
}

int launcher_browser_last_error(void)
{
    return g_last_error;
}

void launcher_browser_move(int delta, int visible_rows)
{
    int max_scroll;

    if (g_entry_count <= 0 && g_scan_done)
        return;

    while (delta > 0) {
        if (g_selected + 1 < g_entry_count) {
            g_selected++;
        } else if (!g_scan_done) {
            int old_count = g_entry_count;
            if (!load_more_entries(LAUNCHER_BROWSER_LOAD_CHUNK))
                break;
            if (g_entry_count == old_count)
                break;
            g_selected++;
        } else {
            break;
        }
        delta--;
    }

    while (delta < 0) {
        if (g_selected > 0) {
            g_selected--;
        } else {
            break;
        }
        delta++;
    }

    if (g_selected < g_scroll)
        g_scroll = g_selected;

    if (g_selected >= g_scroll + visible_rows)
        g_scroll = g_selected - visible_rows + 1;

    max_scroll = g_entry_count - visible_rows;
    if (max_scroll < 0)
        max_scroll = 0;

    if (g_scroll > max_scroll)
        g_scroll = max_scroll;
    if (g_scroll < 0)
        g_scroll = 0;
}

int launcher_browser_activate(char *selected_path, size_t path_size, char *selected_label, size_t label_size)
{
    const launcher_browser_entry_t *entry;
    char full[512];

    if (g_entry_count <= 0)
        return 0;

    entry = launcher_browser_entry(g_selected);
    if (!entry)
        return 0;

    if (selected_label && label_size > 0)
        snprintf(selected_label, label_size, "%s", entry->name);

    if (is_root_path(g_current_path))
        snprintf(full, sizeof(full), "%s", entry->name);
    else
        path_join(full, sizeof(full), g_current_path, entry->name);

    if (entry->is_dir)
        return launcher_browser_open(full) ? 0 : -1;

    if (selected_path && path_size > 0)
        snprintf(selected_path, path_size, "%s", full);

    return 1;
}
