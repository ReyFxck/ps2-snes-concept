#include "launcher_browser_internal.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

launcher_browser_entry_t *g_entries = NULL;
int g_entry_count = 0;
int g_entry_capacity = 0;
char g_current_path[256];
int g_selected = 0;
int g_scroll = 0;
int g_last_error = 0;
DIR *g_scan_dir = NULL;
int g_scan_done = 1;

int launcher_browser_is_root_path(const char *path)
{
    return !path || !path[0];
}

void launcher_browser_close_scan_dir(void)
{
    if (g_scan_dir) {
        closedir(g_scan_dir);
        g_scan_dir = NULL;
    }
}

void launcher_browser_clear_entries(void)
{
    g_entry_count = 0;
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
}

int launcher_browser_ensure_capacity(int need)
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

int launcher_browser_append_entry(const char *name, int is_dir)
{
    if (!launcher_browser_ensure_capacity(g_entry_count + 1))
        return 0;

    snprintf(g_entries[g_entry_count].name, sizeof(g_entries[g_entry_count].name), "%s", name);
    g_entries[g_entry_count].is_dir = is_dir;
    g_entry_count++;
    return 1;
}

static int reset_to_path(const char *path)
{
    snprintf(g_current_path, sizeof(g_current_path), "%s", path ? path : "");
    launcher_browser_clear_entries();

    if (launcher_browser_is_root_path(g_current_path))
        return launcher_browser_scan_root_devices();

    if (!launcher_browser_open_scan_dir(g_current_path))
        return 0;

    return launcher_browser_load_more_entries(LAUNCHER_BROWSER_LOAD_CHUNK);
}

void launcher_browser_init(void)
{
    launcher_browser_close_scan_dir();

    if (g_entries) {
        free(g_entries);
        g_entries = NULL;
    }

    g_entry_count = 0;
    g_entry_capacity = 0;
    g_current_path[0] = '\0';
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
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

    if (launcher_browser_is_root_path(g_current_path))
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
    if (launcher_browser_is_root_path(g_current_path))
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

            if (!launcher_browser_load_more_entries(LAUNCHER_BROWSER_LOAD_CHUNK))
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
        if (g_selected > 0)
            g_selected--;
        else
            break;

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

    if (launcher_browser_is_root_path(g_current_path))
        snprintf(full, sizeof(full), "%s", entry->name);
    else
        launcher_browser_path_join(full, sizeof(full), g_current_path, entry->name);

    if (entry->is_dir)
        return launcher_browser_open(full) ? 0 : -1;

    if (selected_path && path_size > 0)
        snprintf(selected_path, path_size, "%s", full);

    return 1;
}
