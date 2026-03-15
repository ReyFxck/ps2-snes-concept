#include "launcher_browser_internal.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

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

int launcher_browser_scan_root_devices(void)
{
    char probe[8] = "mass0:/";
    int found = 0;
    int i;

    launcher_browser_clear_entries();
    launcher_browser_close_scan_dir();
    g_scan_done = 1;

    for (i = 0; i < LAUNCHER_BROWSER_MAX_USB_DEVICES; i++) {
        probe[4] = (char)('0' + i);

        if (!device_exists(probe))
            continue;

        if (i == 0) {
            if (!launcher_browser_append_entry("mass:/", 1))
                return 0;
        } else {
            char label[8];
            snprintf(label, sizeof(label), "mass%d:/", i);
            if (!launcher_browser_append_entry(label, 1))
                return 0;
        }

        found = 1;
    }

    if (!found)
        g_last_error = 1;

    return found;
}
