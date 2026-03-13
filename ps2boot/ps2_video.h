#ifndef PS2_VIDEO_H
#define PS2_VIDEO_H

#include <stddef.h>
#include <stdint.h>

int ps2_video_init_once(void);
void ps2_video_set_debug(const char *line1, const char *line2, const char *line3, const char *line4);
void ps2_video_present_rgb565(const void *data, unsigned width, unsigned height, size_t pitch);

#endif
