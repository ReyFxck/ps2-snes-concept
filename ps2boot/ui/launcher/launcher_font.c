#include "launcher_font.h"

#include "../../ps2_launcher_video.h"
#include "../select_menu/font/select_menu_font_data.h"

static void launcher_font_draw_char_scaled(unsigned x, unsigned y, char c, uint16_t color, unsigned scale)
{
    int row;
    int col;
    unsigned sx;
    unsigned sy;

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
                        ps2_launcher_video_put_pixel(
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

void launcher_font_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color)
{
    launcher_font_draw_string_color_scaled(x, y, s, color, 1);
}

void launcher_font_draw_string_color_scaled(unsigned x, unsigned y, const char *s, uint16_t color, unsigned scale)
{
    unsigned i;

    if (!s)
        return;

    if (scale == 0)
        scale = 1;

    for (i = 0; s[i]; i++) {
        launcher_font_draw_char_scaled(x + i * (6 * scale) + scale,
                                       y + scale,
                                       s[i],
                                       0x0000,
                                       scale);
        launcher_font_draw_char_scaled(x + i * (6 * scale),
                                       y,
                                       s[i],
                                       color,
                                       scale);
    }
}
