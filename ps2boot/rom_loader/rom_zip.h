#ifndef ROM_ZIP_H
#define ROM_ZIP_H

#include <stddef.h>

int rom_zip_load(const char *zip_path, void **out_data, size_t *out_size, char *out_name, size_t out_name_size);

#endif
