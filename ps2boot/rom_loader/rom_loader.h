#ifndef ROM_LOADER_H
#define ROM_LOADER_H

#include <stddef.h>

int rom_loader_is_supported(const char *path);
int rom_loader_load(const char *path, void **out_data, size_t *out_size, char *out_name, size_t out_name_size);
void rom_loader_free(void **data, size_t *size);

#endif
