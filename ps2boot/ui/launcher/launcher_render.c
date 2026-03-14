#include "launcher_render.h"

#include "../../ps2_launcher_video.h"
#include "launcher_pages.h"
#include "launcher_theme.h"

static void fill_rect(int x, int y, int w, int h, uint16_t color)
{
    int yy;
    int xx;

    for (yy = 0; yy < h; yy++) {
        for (xx = 0; xx < w; xx++) {
            ps2_launcher_video_put_pixel((unsigned)(x + xx), (unsigned)(y + yy), color);
        }
    }
}

void launcher_render(const launcher_state_t *state)
{
    if (!state)
        return;

    if (!ps2_launcher_video_init_once())
        return;

    ps2_launcher_video_begin_frame(LAUNCHER_COLOR_BG);

    fill_rect(0, 0, 640, 4, LAUNCHER_COLOR_BORDER);
    fill_rect(0, 444, 640, 4, LAUNCHER_COLOR_BORDER);
    fill_rect(0, 0, 4, 448, LAUNCHER_COLOR_BORDER);
    fill_rect(636, 0, 4, 448, LAUNCHER_COLOR_BORDER);

    fill_rect(48, 94, 544, 2, LAUNCHER_COLOR_BORDER);
    fill_rect(48, 280, 544, 2, LAUNCHER_COLOR_BORDER);

    launcher_pages_draw(state);
    ps2_launcher_video_end_frame();
}
