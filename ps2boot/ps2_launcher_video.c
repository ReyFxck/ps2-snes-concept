#include "ps2_launcher_video.h"
#include "ps2_video.h"

#include <string.h>
#include <dma.h>
#include <draw.h>
#include <graph.h>
#include <packet.h>
#include <gs_psm.h>

static uint16_t g_launcher_upload[PS2_LAUNCHER_HEIGHT][PS2_LAUNCHER_WIDTH];

static int g_launcher_video_ready = 0;
static texbuffer_t g_launcher_tex;
static packet_t *g_launcher_tex_packet = 0;
static packet_t *g_launcher_draw_packet = 0;

static framebuffer_t g_launcher_frame;
static zbuffer_t g_launcher_z;

int ps2_launcher_video_init_once(void)
{
    packet_t *packet;
    qword_t *q;
    lod_t lod;
    clutbuffer_t clut;

    if (g_launcher_video_ready)
        return 1;

    memset(&g_launcher_frame, 0, sizeof(g_launcher_frame));
    g_launcher_frame.width = ps2_video_frame_width();
    g_launcher_frame.height = ps2_video_frame_height();
    g_launcher_frame.mask = 0;
    g_launcher_frame.psm = GS_PSM_32;
    g_launcher_frame.address = ps2_video_frame_address();

    memset(&g_launcher_z, 0, sizeof(g_launcher_z));
    g_launcher_z.enable = DRAW_DISABLE;

    memset(&g_launcher_tex, 0, sizeof(g_launcher_tex));
    g_launcher_tex.width = 1024;
    g_launcher_tex.psm = GS_PSM_16;
    g_launcher_tex.address = graph_vram_allocate(1024, 512, GS_PSM_16, GRAPH_ALIGN_BLOCK);
    g_launcher_tex.info.width = draw_log2(1024);
    g_launcher_tex.info.height = draw_log2(512);
    g_launcher_tex.info.components = TEXTURE_COMPONENTS_RGB;
    g_launcher_tex.info.function = TEXTURE_FUNCTION_DECAL;

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
    q = draw_texturebuffer(q, 0, &g_launcher_tex, &clut);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    g_launcher_tex_packet = packet_init(256, PACKET_NORMAL);
    g_launcher_draw_packet = packet_init(256, PACKET_NORMAL);

    if (!g_launcher_tex_packet || !g_launcher_draw_packet)
        return 0;

    g_launcher_video_ready = 1;
    return 1;
}

void ps2_launcher_video_begin_frame(uint16_t clear_color)
{
    unsigned y;
    unsigned x;
    uint16_t conv = ps2_video_convert_rgb565(clear_color);

    for (y = 0; y < PS2_LAUNCHER_HEIGHT; y++) {
        for (x = 0; x < PS2_LAUNCHER_WIDTH; x++) {
            g_launcher_upload[y][x] = conv;
        }
    }
}

void ps2_launcher_video_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    if (x >= PS2_LAUNCHER_WIDTH || y >= PS2_LAUNCHER_HEIGHT)
        return;

    g_launcher_upload[y][x] = ps2_video_convert_rgb565(color);
}

void ps2_launcher_video_end_frame(void)
{
    qword_t *q;
    texrect_t rect;

    dma_wait_fast();

    q = g_launcher_tex_packet->data;
    q = draw_texture_transfer(q,
                              g_launcher_upload,
                              PS2_LAUNCHER_WIDTH,
                              PS2_LAUNCHER_HEIGHT,
                              GS_PSM_16,
                              g_launcher_tex.address,
                              g_launcher_tex.width);
    q = draw_texture_flush(q);

    dma_channel_send_chain(DMA_CHANNEL_GIF,
                           g_launcher_tex_packet->data,
                           q - g_launcher_tex_packet->data,
                           0,
                           0);
    dma_wait_fast();

    memset(&rect, 0, sizeof(rect));

    rect.v0.x = 0.0f;
    rect.v0.y = 0.0f;
    rect.v0.z = 0;

    rect.v1.x = 640.0f;
    rect.v1.y = 448.0f;
    rect.v1.z = 0;

    rect.t0.u = 0.0f;
    rect.t0.v = 0.0f;
    rect.t1.u = (float)PS2_LAUNCHER_WIDTH - 1.0f;
    rect.t1.v = (float)PS2_LAUNCHER_HEIGHT - 1.0f;

    rect.color.r = 0x80;
    rect.color.g = 0x80;
    rect.color.b = 0x80;
    rect.color.a = 0x80;
    rect.color.q = 1.0f;

    q = g_launcher_draw_packet->data;
    q = draw_setup_environment(q, 0, &g_launcher_frame, &g_launcher_z);
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_launcher_frame.width, (float)g_launcher_frame.height, 0, 0, 0);
    q = draw_rect_textured(q, 0, &rect);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF,
                            g_launcher_draw_packet->data,
                            q - g_launcher_draw_packet->data,
                            0,
                            0);
    draw_wait_finish();
    graph_wait_vsync();
}
