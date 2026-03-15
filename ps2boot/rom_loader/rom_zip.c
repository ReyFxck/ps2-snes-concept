#include "rom_zip.h"

#include <stdlib.h>
#include <string.h>

#include "miniz/miniz.h"

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

static const char *base_name_only(const char *path)
{
    const char *a;
    const char *b;
    const char *p;

    if (!path)
        return "";

    a = strrchr(path, '/');
    b = strrchr(path, '\\');

    p = a;
    if (!p || (b && b > p))
        p = b;

    return p ? (p + 1) : path;
}

static int zip_name_is_rom(const char *name)
{
    if (ext_equals(name, ".smc"))
        return 1;
    if (ext_equals(name, ".sfc"))
        return 1;
    if (ext_equals(name, ".swc"))
        return 1;
    if (ext_equals(name, ".fig"))
        return 1;

    return 0;
}

int rom_zip_load(const char *zip_path, void **out_data, size_t *out_size, char *out_name, size_t out_name_size)
{
    mz_zip_archive zip;
    mz_uint i;
    mz_uint file_count;

    if (!zip_path || !out_data || !out_size)
        return 0;

    *out_data = NULL;
    *out_size = 0;

    if (out_name && out_name_size > 0)
        out_name[0] = '\0';

    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_file(&zip, zip_path, 0))
        return 0;

    file_count = mz_zip_reader_get_num_files(&zip);

    for (i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat st;
        void *buf;

        memset(&st, 0, sizeof(st));

        if (!mz_zip_reader_file_stat(&zip, i, &st))
            continue;

        if (mz_zip_reader_is_file_a_directory(&zip, i))
            continue;

        if (!zip_name_is_rom(st.m_filename))
            continue;

        if (!st.m_uncomp_size)
            continue;

        if (st.m_uncomp_size > (mz_uint64)((size_t)-1))
            continue;

        buf = malloc((size_t)st.m_uncomp_size);
        if (!buf)
            break;

        if (!mz_zip_reader_extract_to_mem(&zip, i, buf, (size_t)st.m_uncomp_size, 0)) {
            free(buf);
            continue;
        }

        *out_data = buf;
        *out_size = (size_t)st.m_uncomp_size;

        if (out_name && out_name_size > 0) {
            strncpy(out_name, base_name_only(st.m_filename), out_name_size - 1);
            out_name[out_name_size - 1] = '\0';
        }

        mz_zip_reader_end(&zip);
        return 1;
    }

    mz_zip_reader_end(&zip);
    return 0;
}
