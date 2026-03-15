#include "rom_loader.h"
#include "rom_zip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char to_lower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

static int ext_equals(const char *path, const char *ext)
{
    const char *dot;
    size_t i;

    if (!path || !ext)
        return 0;

    dot = strrchr(path, '.');
    if (!dot)
        return 0;

    for (i = 0; dot[i] && ext[i]; i++) {
        if (to_lower_ascii(dot[i]) != to_lower_ascii(ext[i]))
            return 0;
    }

    return dot[i] == '\0' && ext[i] == '\0';
}

static int load_plain_file(const char *path, void **out_data, size_t *out_size)
{
    FILE *fp;
    long file_size;
    size_t read_bytes;
    void *buf;

    fp = fopen(path, "rb");
    if (!fp)
        return 0;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return 0;
    }

    file_size = ftell(fp);
    if (file_size <= 0) {
        fclose(fp);
        return 0;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return 0;
    }

    buf = malloc((size_t)file_size);
    if (!buf) {
        fclose(fp);
        return 0;
    }

    read_bytes = fread(buf, 1, (size_t)file_size, fp);
    fclose(fp);

    if (read_bytes != (size_t)file_size) {
        free(buf);
        return 0;
    }

    *out_data = buf;
    *out_size = (size_t)file_size;
    return 1;
}

int rom_loader_is_supported(const char *path)
{
    if (!path || !path[0])
        return 0;

    if (ext_equals(path, ".smc"))
        return 1;
    if (ext_equals(path, ".sfc"))
        return 1;
    if (ext_equals(path, ".swc"))
        return 1;
    if (ext_equals(path, ".fig"))
        return 1;
    if (ext_equals(path, ".zip"))
        return 1;

    return 0;
}

int rom_loader_load(const char *path, void **out_data, size_t *out_size, char *out_name, size_t out_name_size)
{
    if (!path || !path[0] || !out_data || !out_size)
        return 0;

    *out_data = NULL;
    *out_size = 0;

    if (out_name && out_name_size > 0)
        out_name[0] = '\0';

    if (ext_equals(path, ".zip"))
        return rom_zip_load(path, out_data, out_size, out_name, out_name_size);

    if (!rom_loader_is_supported(path))
        return 0;

    return load_plain_file(path, out_data, out_size);
}

void rom_loader_free(void **data, size_t *size)
{
    if (data && *data) {
        free(*data);
        *data = NULL;
    }

    if (size)
        *size = 0;
}
