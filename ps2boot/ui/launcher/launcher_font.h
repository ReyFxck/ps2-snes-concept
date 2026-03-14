#ifndef LAUNCHER_FONT_H
#define LAUNCHER_FONT_H

#include <stdint.h>

void launcher_font_draw_string_color_scaled(unsigned x, unsigned y, const char *s, uint16_t color, unsigned scale);
void launcher_font_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color);

#endif
