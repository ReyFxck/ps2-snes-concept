#include "launcher_browser_internal.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int launcher_browser_has_rom_ext(const char *name)
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

int launcher_browser_open_scan_dir(const char *path)
{
    launcher_browser_close_scan_dir();

    g_scan_dir = opendir(path);
    if (!g_scan_dir) {
        g_last_error = 1;
        g_scan_done = 1;
        return 0;
    }

    g_scan_done = 0;
    return 1;
}

void launcher_browser_path_join(char *out, size_t out_size, const char *base, const char *name)
{
    size_t len = strlen(base);

    if (len > 0 && base[len - 1] == '/')
        snprintf(out, out_size, "%s%s", base, name);
    else
        snprintf(out, out_size, "%s/%s", base, name);
}

int launcher_browser_load_more_entries(int want)
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
            launcher_browser_close_scan_dir();
            g_scan_done = 1;
            break;
        }

        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        launcher_browser_path_join(full, sizeof(full), g_current_path, de->d_name);

        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            keep = 1;
            is_dir = 1;
        } else if (launcher_browser_has_rom_ext(de->d_name)) {
            keep = 1;
            is_dir = 0;
        }

        if (!keep)
            continue;

        if (!launcher_browser_append_entry(de->d_name, is_dir))
            return 0;

        added++;
    }

    return 1;
}
