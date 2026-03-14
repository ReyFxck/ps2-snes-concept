#include "select_menu_font.h"

#include "../../../ps2_video.h"
#include "select_menu_font_data.h"

static void select_menu_font_draw_char_scaled(unsigned x, unsigned y, char c, uint16_t color, unsigned scale)
{
    int row;
    int col;
    unsigned sy;
    unsigned sx;

    if (scale == 0)
        scale = 1;

    if (c >= 'a' && c <= 'z')
        c = (char)(c - ('a' - 'A'));

    for (row = 0; row < 7; row++) {
        uint8_t bits = select_menu_font_glyph_row(c, row);

        for (col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col))) {
                for (sy = 0; sy < scale; sy++) {
                    for (sx = 0; sx < scale; sx++) {
                        ps2_video_ui_put_pixel(
                            x + (unsigned)col * scale + sx,
                            y + (unsigned)row * scale + sy,
                            color
                        );
                    }
                }
            }
        }
    }
}

void select_menu_font_draw_string(unsigned x, unsigned y, const char *s)
{
    select_menu_font_draw_string_color(x, y, s, 0xFFFF);
}

void select_menu_font_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color)
{
    select_menu_font_draw_string_color_scaled(x, y, s, color, 1);
}

void select_menu_font_draw_string_color_scaled(unsigned x, unsigned y, const char *s, uint16_t color, unsigned scale)
{
    unsigned i;
    uint16_t shadow = 0x8000;

    if (!s)
        return;

    if (scale == 0)
        scale = 1;

    for (i = 0; s[i]; i++) {
        select_menu_font_draw_char_scaled(x + i * (6 * scale) + scale, y + scale, s[i], shadow, scale);
        select_menu_font_draw_char_scaled(x + i * (6 * scale), y, s[i], color, scale);
    }
}
