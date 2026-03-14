#ifndef PS2_VIDEO_H
#define PS2_VIDEO_H

#include <stddef.h>
#include <stdint.h>

#define PS2_LAUNCHER_WIDTH 640
#define PS2_LAUNCHER_HEIGHT 448

enum
{
    PS2_MENU_MAIN = 0,
    PS2_MENU_VIDEO = 1,
    PS2_MENU_VIDEO_DISPLAY = 2,
    PS2_MENU_VIDEO_ASPECT = 3
};

enum
{
    PS2_ASPECT_4_3 = 0,
    PS2_ASPECT_16_9 = 1,
    PS2_ASPECT_FULL = 2,
    PS2_ASPECT_PIXEL = 3
};

int ps2_video_init_once(void);
void ps2_video_set_debug(const char *line1, const char *line2, const char *line3, const char *line4);
void ps2_video_set_offsets(int x, int y);
void ps2_video_get_offsets(int *x, int *y);
void ps2_video_set_aspect(int mode);
int ps2_video_get_aspect(void);
uint32_t ps2_video_frame_address(void);
unsigned ps2_video_frame_width(void);
unsigned ps2_video_frame_height(void);
uint16_t ps2_video_convert_rgb565(uint16_t color);

void ps2_video_ui_set_menu_target(void);
void ps2_video_ui_put_pixel(unsigned x, unsigned y, uint16_t color);
uint16_t ps2_video_ui_get_pixel(unsigned x, unsigned y);

void ps2_video_menu_begin_frame(void);
void ps2_video_menu_put_pixel(unsigned x, unsigned y, uint16_t color);
void ps2_video_menu_end_frame(void);

void ps2_video_launcher_begin_frame(uint16_t clear_color);
void ps2_video_launcher_end_frame(void);

void ps2_video_draw_menu(int page, int main_sel, int video_sel, int aspect_sel);
void ps2_video_present_rgb565(const void *data, unsigned width, unsigned height, size_t pitch);

#endif
