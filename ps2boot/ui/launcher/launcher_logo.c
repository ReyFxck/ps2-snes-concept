#include "launcher_logo.h"

#include <stdint.h>
#include "../../ps2_launcher_video.h"

extern unsigned char launcher_logo_rgba[];

static uint16_t pack_rgb565(unsigned r, unsigned g, unsigned b)
{
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void launcher_logo_draw(int dst_x, int dst_y)
{
    int y;
    int x;

    for (y = 0; y < LAUNCHER_LOGO_HEIGHT; y++) {
        for (x = 0; x < LAUNCHER_LOGO_WIDTH; x++) {
            unsigned idx = (unsigned)((y * LAUNCHER_LOGO_WIDTH + x) * 4);
            unsigned sr = launcher_logo_rgba[idx + 0];
            unsigned sg = launcher_logo_rgba[idx + 1];
            unsigned sb = launcher_logo_rgba[idx + 2];
            unsigned sa = launcher_logo_rgba[idx + 3];

            if (sa < 128)
                continue;

            ps2_launcher_video_put_pixel((unsigned)(dst_x + x),
                                         (unsigned)(dst_y + y),
                                         pack_rgb565(sr, sg, sb));
        }
    }
}
