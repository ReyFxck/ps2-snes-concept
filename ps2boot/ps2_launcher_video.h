#ifndef PS2_LAUNCHER_VIDEO_H
#define PS2_LAUNCHER_VIDEO_H

#include <stdint.h>

#define PS2_LAUNCHER_WIDTH 640
#define PS2_LAUNCHER_HEIGHT 448

int ps2_launcher_video_init_once(void);
void ps2_launcher_video_begin_frame(uint16_t clear_color);
void ps2_launcher_video_put_pixel(unsigned x, unsigned y, uint16_t color);
void ps2_launcher_video_end_frame(void);

#endif
