#include "ps2_video.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <gs_psm.h>

#define VIDEO_WAIT_VSYNC 0

static int g_video_ready = 0;
static int g_lut_ready = 0;
static int g_video_off_x = 0;
static int g_video_off_y = 0;
static int g_aspect_mode = PS2_ASPECT_4_3;

static framebuffer_t g_frame;
static zbuffer_t g_z;
static texbuffer_t g_tex;
static packet_t *g_tex_packet = 0;
static packet_t *g_draw_packet = 0;

static uint16_t g_upload[256 * 224] __attribute__((aligned(64)));
static uint16_t g_frame_base[256 * 224] __attribute__((aligned(64)));
static uint16_t g_rgb565_lut[65536] __attribute__((aligned(64)));

static char g_dbg1[48] = "";
static char g_dbg2[48] = "";
static char g_dbg3[48] = "";
static char g_dbg4[48] = "";

static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void ps2_video_build_lut(void)
{
    unsigned i;

    if (g_lut_ready)
        return;

    for (i = 0; i < 65536; i++) {
        uint16_t c = (uint16_t)i;
        uint16_t r = (c >> 11) & 0x1f;
        uint16_t g = (c >> 5)  & 0x3f;
        uint16_t b =  c        & 0x1f;

        g >>= 1;

        g_rgb565_lut[i] = (1u << 15) | (b << 10) | (g << 5) | r;
    }

    g_lut_ready = 1;
}

void ps2_video_set_debug(const char *line1, const char *line2, const char *line3, const char *line4)
{
    if (line1) { strncpy(g_dbg1, line1, sizeof(g_dbg1) - 1); g_dbg1[sizeof(g_dbg1) - 1] = 0; }
    if (line2) { strncpy(g_dbg2, line2, sizeof(g_dbg2) - 1); g_dbg2[sizeof(g_dbg2) - 1] = 0; }
    if (line3) { strncpy(g_dbg3, line3, sizeof(g_dbg3) - 1); g_dbg3[sizeof(g_dbg3) - 1] = 0; }
    if (line4) { strncpy(g_dbg4, line4, sizeof(g_dbg4) - 1); g_dbg4[sizeof(g_dbg4) - 1] = 0; }
}

static void ps2_video_apply_display_offset(void)
{
    graph_set_screen(g_video_off_x, g_video_off_y, g_frame.width, g_frame.height);
}

void ps2_video_set_offsets(int x, int y)
{
    g_video_off_x = clamp_int(x, -96, 96);
    g_video_off_y = clamp_int(y, -64, 64);

    if (g_video_ready)
        ps2_video_apply_display_offset();
}

void ps2_video_get_offsets(int *x, int *y)
{
    if (x) *x = g_video_off_x;
    if (y) *y = g_video_off_y;
}

void ps2_video_set_aspect(int mode)
{
    if (mode < PS2_ASPECT_4_3)
        mode = PS2_ASPECT_4_3;
    if (mode > PS2_ASPECT_PIXEL)
        mode = PS2_ASPECT_PIXEL;

    g_aspect_mode = mode;
}

int ps2_video_get_aspect(void)
{
    return g_aspect_mode;
}

static uint8_t dbg_glyph_row(char c, int row)
{
    switch (c) {
        case '0': { static const uint8_t g[7] = {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}; return g[row]; }
        case '1': { static const uint8_t g[7] = {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}; return g[row]; }
        case '2': { static const uint8_t g[7] = {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}; return g[row]; }
        case '3': { static const uint8_t g[7] = {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}; return g[row]; }
        case '4': { static const uint8_t g[7] = {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}; return g[row]; }
        case '5': { static const uint8_t g[7] = {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}; return g[row]; }
        case '6': { static const uint8_t g[7] = {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}; return g[row]; }
        case '7': { static const uint8_t g[7] = {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}; return g[row]; }
        case '8': { static const uint8_t g[7] = {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}; return g[row]; }
        case '9': { static const uint8_t g[7] = {0x0E,0x11,0x11,0x0F,0x01,0x02,0x1C}; return g[row]; }
        case 'A': { static const uint8_t g[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; return g[row]; }
        case 'B': { static const uint8_t g[7] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}; return g[row]; }
        case 'C': { static const uint8_t g[7] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}; return g[row]; }
        case 'D': { static const uint8_t g[7] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}; return g[row]; }
        case 'E': { static const uint8_t g[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; return g[row]; }
        case 'F': { static const uint8_t g[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}; return g[row]; }
        case 'G': { static const uint8_t g[7] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}; return g[row]; }
        case 'I': { static const uint8_t g[7] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}; return g[row]; }
        case 'K': { static const uint8_t g[7] = {0x11,0x12,0x14,0x18,0x14,0x12,0x11}; return g[row]; }
        case 'L': { static const uint8_t g[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; return g[row]; }
        case 'M': { static const uint8_t g[7] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}; return g[row]; }
        case 'N': { static const uint8_t g[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11}; return g[row]; }
        case 'O': { static const uint8_t g[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; return g[row]; }
        case 'P': { static const uint8_t g[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; return g[row]; }
        case 'R': { static const uint8_t g[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; return g[row]; }
        case 'S': { static const uint8_t g[7] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; return g[row]; }
        case 'T': { static const uint8_t g[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; return g[row]; }
        case 'U': { static const uint8_t g[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}; return g[row]; }
        case 'V': { static const uint8_t g[7] = {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}; return g[row]; }
        case 'X': { static const uint8_t g[7] = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}; return g[row]; }
        case 'Y': { static const uint8_t g[7] = {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}; return g[row]; }
        case '=': { static const uint8_t g[7] = {0x00,0x1F,0x00,0x1F,0x00,0x00,0x00}; return g[row]; }
        case '-': { static const uint8_t g[7] = {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}; return g[row]; }
        case '>': { static const uint8_t g[7] = {0x10,0x08,0x04,0x02,0x04,0x08,0x10}; return g[row]; }
        case ' ':
        default:
            return 0x00;
    }
}

static void dbg_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    if (x >= 256 || y >= 224)
        return;

    g_upload[y * 256 + x] = color;
}

static void dbg_draw_char(unsigned x, unsigned y, char c, uint16_t color)
{
    int row, col;

    for (row = 0; row < 7; row++) {
        uint8_t bits = dbg_glyph_row(c, row);
        for (col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col)))
                dbg_put_pixel(x + (unsigned)col, y + (unsigned)row, color);
        }
    }
}

static void dbg_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color)
{
    unsigned i;
    uint16_t shadow = 0x8000;

    for (i = 0; s[i]; i++) {
        dbg_draw_char(x + i * 6 + 1, y + 1, s[i], shadow);
        dbg_draw_char(x + i * 6,     y,     s[i], color);
    }
}

static void dbg_draw_string(unsigned x, unsigned y, const char *s)
{
    dbg_draw_string_color(x, y, s, 0xFFFF);
}

static void dbg_overlay(void)
{
    if (!g_dbg1[0] && !g_dbg2[0] && !g_dbg3[0] && !g_dbg4[0])
        return;

    dbg_draw_string(2,  2, g_dbg1);
    dbg_draw_string(2, 10, g_dbg2);
    dbg_draw_string(2, 18, g_dbg3);
    dbg_draw_string(2, 26, g_dbg4);
}

static void menu_tint_blue(void)
{
    unsigned i;

    for (i = 0; i < 256u * 224u; i++) {
        uint16_t c = g_upload[i];
        uint16_t r =  c        & 0x1f;
        uint16_t g = (c >> 5)  & 0x1f;
        uint16_t b = (c >> 10) & 0x1f;

        r = (uint16_t)((r * 2) / 5);
        g = (uint16_t)((g * 2) / 5);
        b = (uint16_t)clamp_int((int)((b * 3) / 5) + 10, 0, 31);

        g_upload[i] = (1u << 15) | (b << 10) | (g << 5) | r;
    }
}

static void ps2_video_upload_and_draw_bound(unsigned width, unsigned height, int wait_vsync);
static void ps2_video_upload_and_draw(unsigned width, unsigned height, int wait_vsync)
{
    qword_t *q;
    texrect_t rect;
    float x0, y0, x1, y1;

    dma_wait_fast();

    q = g_tex_packet->data;
    q = draw_texture_transfer(q, g_upload, 256, 224, GS_PSM_16, g_tex.address, g_tex.width);
    q = draw_texture_flush(q);
    dma_channel_send_chain(DMA_CHANNEL_GIF, g_tex_packet->data, q - g_tex_packet->data, 0, 0);
    dma_wait_fast();

    switch (g_aspect_mode) {
        case PS2_ASPECT_16_9:
            x0 = 0.0f;
            y0 = 44.0f;
            x1 = 640.0f;
            y1 = 404.0f;
            break;

        case PS2_ASPECT_FULL:
            x0 = 0.0f;
            y0 = 0.0f;
            x1 = 640.0f;
            y1 = 448.0f;
            break;

        case PS2_ASPECT_PIXEL:
            x0 = 96.0f;
            y0 = 28.0f;
            x1 = 544.0f;
            y1 = 420.0f;
            break;

        case PS2_ASPECT_4_3:
        default:
            x0 = 64.0f;
            y0 = 0.0f;
            x1 = 576.0f;
            y1 = 448.0f;
            break;
    }

    memset(&rect, 0, sizeof(rect));

    rect.v0.x = x0;
    rect.v0.y = y0;
    rect.v0.z = 0;

    rect.v1.x = x1;
    rect.v1.y = y1;
    rect.v1.z = 0;

    rect.t0.u = 0.0f;
    rect.t0.v = 0.0f;
    rect.t1.u = (float)width - 1.0f;
    rect.t1.v = (float)height - 1.0f;

    rect.color.r = 0x80;
    rect.color.g = 0x80;
    rect.color.b = 0x80;
    rect.color.a = 0x80;
    rect.color.q = 1.0f;

    q = g_draw_packet->data;
    q = draw_setup_environment(q, 0, &g_frame, &g_z);
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_frame.width, (float)g_frame.height, 0, 0, 0);
    q = draw_rect_textured(q, 0, &rect);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, g_draw_packet->data, q - g_draw_packet->data, 0, 0);
    draw_wait_finish();

    if (wait_vsync)
        graph_wait_vsync();
}


void ps2_video_menu_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    if (x >= 256 || y >= 224)
        return;

    g_upload[y * 256 + x] = color;
}

void ps2_video_menu_begin_frame(void)
{
    memcpy(g_upload, g_frame_base, sizeof(g_upload));
    menu_tint_blue();
}

void ps2_video_menu_end_frame(void)
{
    ps2_video_upload_and_draw_bound(256, 224, 1);
}

void ps2_video_draw_menu(int page, int main_sel, int video_sel, int aspect_sel)
{
    char buf_x[40];
    char buf_y[40];
    uint16_t white  = 0xFFFF;
    uint16_t yellow = 0x83FF;

    (void)main_sel;

    memcpy(g_upload, g_frame_base, sizeof(g_upload));
    menu_tint_blue();

    if (page == PS2_MENU_MAIN) {
        dbg_draw_string_color(115, 18, "MENU", white);

        dbg_draw_string_color(104, 92,
                              main_sel == 0 ? "> RESUME" : "  RESUME",
                              main_sel == 0 ? yellow : white);

        dbg_draw_string_color(116, 110,
                              main_sel == 1 ? "> VIDEO" : "  VIDEO",
                              main_sel == 1 ? yellow : white);

        dbg_draw_string_color(8, 194, "SELECT CLOSE", white);
    }
    else if (page == PS2_MENU_VIDEO) {
        dbg_draw_string_color(110, 18, "VIDEO", white);

        dbg_draw_string_color(62, 88,
                              video_sel == 0 ? "> DISPLAY POSITION" : "  DISPLAY POSITION",
                              video_sel == 0 ? yellow : white);

        dbg_draw_string_color(80, 106,
                              video_sel == 1 ? "> ASPECT RATIO" : "  ASPECT RATIO",
                              video_sel == 1 ? yellow : white);

        dbg_draw_string_color(116, 124,
                              video_sel == 2 ? "> BACK" : "  BACK",
                              video_sel == 2 ? yellow : white);

        dbg_draw_string_color(8, 180, "CROSS = OPEN", white);
        dbg_draw_string_color(8, 194, "SELECT CLOSE", white);
    }
    else if (page == PS2_MENU_VIDEO_DISPLAY) {
        dbg_draw_string_color(74, 18, "DISPLAY POSITION", white);
        dbg_draw_string_color(80, 74, "D-PAD MOVES OUTPUT", white);

        snprintf(buf_x, sizeof(buf_x), "X = %d", g_video_off_x);
        snprintf(buf_y, sizeof(buf_y), "Y = %d", g_video_off_y);

        dbg_draw_string_color(104, 94, buf_x, yellow);
        dbg_draw_string_color(104, 108, buf_y, yellow);

        dbg_draw_string_color(47, 130, "CROSS START CIRCLE = BACK", white);
        dbg_draw_string_color(8, 194, "SELECT CLOSE", white);
    }
    else {
        dbg_draw_string_color(86, 18, "ASPECT RATIO", white);

        dbg_draw_string_color(118, 74,
                              aspect_sel == 0 ? "> 4:3" : "  4:3",
                              aspect_sel == 0 ? yellow : white);

        dbg_draw_string_color(114, 88,
                              aspect_sel == 1 ? "> 16:9" : "  16:9",
                              aspect_sel == 1 ? yellow : white);

        dbg_draw_string_color(74, 102,
                              aspect_sel == 2 ? "> FULL SCREEN" : "  FULL SCREEN",
                              aspect_sel == 2 ? yellow : white);

        dbg_draw_string_color(68, 116,
                              aspect_sel == 3 ? "> PIXEL PERFECT" : "  PIXEL PERFECT",
                              aspect_sel == 3 ? yellow : white);

        dbg_draw_string_color(114, 130,
                              aspect_sel == 4 ? "> BACK" : "  BACK",
                              aspect_sel == 4 ? yellow : white);

        dbg_draw_string_color(38, 170, "CROSS START = APPLY", white);
        dbg_draw_string_color(8, 194, "SELECT CLOSE", white);
    }

    ps2_video_upload_and_draw_bound(256, 224, 1);
}

int ps2_video_init_once(void)
{
    packet_t *packet;
    qword_t *q;
    lod_t lod;
    clutbuffer_t clut;

    if (g_video_ready)
        return 1;

    ps2_video_build_lut();

    dma_channel_initialize(DMA_CHANNEL_GIF, 0, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    g_frame.width = 640;
    g_frame.height = 448;
    g_frame.mask = 0;
    g_frame.psm = GS_PSM_32;
    g_frame.address = graph_vram_allocate(
        g_frame.width, g_frame.height, g_frame.psm, GRAPH_ALIGN_PAGE
    );

    memset(&g_z, 0, sizeof(g_z));
    g_z.enable = DRAW_DISABLE;

    g_tex.width = 256;
    g_tex.psm = GS_PSM_16;
    g_tex.address = graph_vram_allocate(256, 256, GS_PSM_16, GRAPH_ALIGN_BLOCK);
    g_tex.info.width = draw_log2(256);
    g_tex.info.height = draw_log2(256);
    g_tex.info.components = TEXTURE_COMPONENTS_RGB;
    g_tex.info.function = TEXTURE_FUNCTION_DECAL;

    graph_initialize(
        g_frame.address, g_frame.width, g_frame.height, g_frame.psm, 0, 0
    );

    ps2_video_apply_display_offset();

    packet = packet_init(32, PACKET_NORMAL);
    if (!packet)
        return 0;

    q = packet->data;
    q = draw_setup_environment(q, 0, &g_frame, &g_z);
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_frame.width, (float)g_frame.height, 0, 0, 0);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    memset(&lod, 0, sizeof(lod));
    lod.calculation = LOD_USE_K;
    lod.max_level = 0;
    lod.mag_filter = LOD_MAG_NEAREST;
    lod.min_filter = LOD_MIN_NEAREST;
    lod.l = 0;
    lod.k = 0.0f;

    memset(&clut, 0, sizeof(clut));
    clut.storage_mode = CLUT_STORAGE_MODE1;
    clut.load_method = CLUT_NO_LOAD;

    packet = packet_init(16, PACKET_NORMAL);
    if (!packet)
        return 0;

    q = packet->data;
    q = draw_texture_sampling(q, 0, &lod);
    q = draw_texturebuffer(q, 0, &g_tex, &clut);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    g_tex_packet = packet_init(128, PACKET_NORMAL);
    g_draw_packet = packet_init(128, PACKET_NORMAL);

    if (!g_tex_packet || !g_draw_packet)
        return 0;

    g_video_ready = 1;
    return 1;
}

void ps2_video_present_rgb565(const void *data, unsigned width, unsigned height, size_t pitch)
{
    const uint8_t *src = (const uint8_t *)data;
    unsigned y;

    if (!g_video_ready || !data || width == 0 || height == 0)
        return;

    if (width > 256)
        width = 256;
    if (height > 224)
        height = 224;

    for (y = 0; y < height; y++) {
        const uint16_t *line = (const uint16_t *)(src + (y * pitch));
        uint16_t *dst = &g_upload[y * 256];
        unsigned x;

        for (x = 0; x < width; x++) {
            dst[x] = g_rgb565_lut[line[x]];
        }
    }

    memcpy(g_frame_base, g_upload, sizeof(g_upload));

    dbg_overlay();
    ps2_video_upload_and_draw_bound(width, height, VIDEO_WAIT_VSYNC);
}

uint16_t ps2_video_menu_get_pixel(unsigned x, unsigned y)
{
    if (x >= 256 || y >= 224)
        return 0;

    return g_upload[y * 256 + x];
}

/* launcher/ui target helpers */
static uint16_t g_launcher_upload[PS2_LAUNCHER_HEIGHT][PS2_LAUNCHER_WIDTH];
static int g_ui_target_launcher = 0;

void ps2_video_ui_set_menu_target(void)
{
    g_ui_target_launcher = 0;
}

void ps2_video_ui_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    if (g_ui_target_launcher) {
        if (x >= PS2_LAUNCHER_WIDTH || y >= PS2_LAUNCHER_HEIGHT)
            return;
        g_launcher_upload[y][x] = color;
        return;
    }

    if (x >= 256 || y >= 224)
        return;

    g_upload[y * 256 + x] = color;
}

uint16_t ps2_video_ui_get_pixel(unsigned x, unsigned y)
{
    if (g_ui_target_launcher) {
        if (x >= PS2_LAUNCHER_WIDTH || y >= PS2_LAUNCHER_HEIGHT)
            return 0;
        return g_launcher_upload[y][x];
    }

    if (x >= 256 || y >= 224)
        return 0;

    return g_upload[y * 256 + x];
}

void ps2_video_launcher_begin_frame(uint16_t clear_color)
{
    unsigned y;
    unsigned x;

    g_ui_target_launcher = 1;

    for (y = 0; y < PS2_LAUNCHER_HEIGHT; y++) {
        for (x = 0; x < PS2_LAUNCHER_WIDTH; x++) {
            g_launcher_upload[y][x] = clear_color;
        }
    }
}

void ps2_video_launcher_end_frame(void)
{
    ps2_video_present_rgb565(g_launcher_upload,
                             PS2_LAUNCHER_WIDTH,
                             PS2_LAUNCHER_HEIGHT,
                             PS2_LAUNCHER_WIDTH * 2);

    g_ui_target_launcher = 0;
}

uint32_t ps2_video_frame_address(void)
{
    return g_frame.address;
}

unsigned ps2_video_frame_width(void)
{
    return g_frame.width;
}

unsigned ps2_video_frame_height(void)
{
    return g_frame.height;
}




uint16_t ps2_video_convert_rgb565(uint16_t color)
{
    return g_rgb565_lut[color];
}

static void ps2_video_upload_and_draw_bound(unsigned width, unsigned height, int wait_vsync)
{
    qword_t *q;
    texrect_t rect;
    float x0, y0, x1, y1;
    lod_t lod;
    clutbuffer_t clut;

    dma_wait_fast();

    q = g_tex_packet->data;
    q = draw_texture_transfer(q, g_upload, 256, 224, GS_PSM_16, g_tex.address, g_tex.width);
    q = draw_texture_flush(q);
    dma_channel_send_chain(DMA_CHANNEL_GIF, g_tex_packet->data, q - g_tex_packet->data, 0, 0);
    dma_wait_fast();

    switch (g_aspect_mode) {
        case PS2_ASPECT_16_9:
            x0 = 0.0f;
            y0 = 44.0f;
            x1 = 640.0f;
            y1 = 404.0f;
            break;

        case PS2_ASPECT_FULL:
            x0 = 0.0f;
            y0 = 0.0f;
            x1 = 640.0f;
            y1 = 448.0f;
            break;

        case PS2_ASPECT_PIXEL:
            x0 = 96.0f;
            y0 = 28.0f;
            x1 = 544.0f;
            y1 = 420.0f;
            break;

        case PS2_ASPECT_4_3:
        default:
            x0 = 64.0f;
            y0 = 0.0f;
            x1 = 576.0f;
            y1 = 448.0f;
            break;
    }

    memset(&rect, 0, sizeof(rect));

    rect.v0.x = x0;
    rect.v0.y = y0;
    rect.v0.z = 0;

    rect.v1.x = x1;
    rect.v1.y = y1;
    rect.v1.z = 0;

    rect.t0.u = 0.0f;
    rect.t0.v = 0.0f;
    rect.t1.u = (float)width - 1.0f;
    rect.t1.v = (float)height - 1.0f;

    rect.color.r = 0x80;
    rect.color.g = 0x80;
    rect.color.b = 0x80;
    rect.color.a = 0x80;
    rect.color.q = 1.0f;

    memset(&lod, 0, sizeof(lod));
    lod.calculation = LOD_USE_K;
    lod.max_level = 0;
    lod.mag_filter = LOD_MAG_NEAREST;
    lod.min_filter = LOD_MIN_NEAREST;
    lod.l = 0;
    lod.k = 0.0f;

    memset(&clut, 0, sizeof(clut));
    clut.storage_mode = CLUT_STORAGE_MODE1;
    clut.load_method = CLUT_NO_LOAD;

    q = g_draw_packet->data;
    q = draw_setup_environment(q, 0, &g_frame, &g_z);
    q = draw_texture_sampling(q, 0, &lod);
    q = draw_texturebuffer(q, 0, &g_tex, &clut);
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_frame.width, (float)g_frame.height, 0, 0, 0);
    q = draw_rect_textured(q, 0, &rect);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, g_draw_packet->data, q - g_draw_packet->data, 0, 0);
    draw_wait_finish();

    if (wait_vsync)
        graph_wait_vsync();
}
