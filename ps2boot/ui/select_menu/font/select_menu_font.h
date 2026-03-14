#ifndef SELECT_MENU_FONT_H
#define SELECT_MENU_FONT_H

#include <stdint.h>

void select_menu_font_draw_string(unsigned x, unsigned y, const char *s);
void select_menu_font_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color);
void select_menu_font_draw_string_color_scaled(unsigned x, unsigned y, const char *s, uint16_t color, unsigned scale);

#endif
