#include <tamtypes.h>
#include <kernel.h>
#include <debug.h>
#include <sifrpc.h>

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "libretro.h"
#include "ps2_video.h"
#include "ps2_input.h"

extern unsigned char smw_sfc_start[];
extern unsigned char smw_sfc_end[];

#define AUTO_INPUT_TEST 0

static unsigned g_frame_count = 0;
static unsigned g_input_tick  = 0;
static enum retro_pixel_format g_pixel_format = RETRO_PIXEL_FORMAT_RGB565;

static void die(const char *msg)
{
    scr_printf("\n[ERRO] %s\n", msg);
    SleepThread();
    while (1) {}
}

static int auto_hold(unsigned tick, unsigned a, unsigned b)
{
    return (tick >= a && tick < b);
}

static bool environ_cb(unsigned cmd, void *data)
{
    switch (cmd) {
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            g_pixel_format = *(const enum retro_pixel_format *)data;
            scr_printf("[ENV] pixel format = %d\n", g_pixel_format);
            return true;

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        {
            const char **dir = (const char **)data;
            *dir = "";
            return true;
        }

        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        {
            const char **dir = (const char **)data;
            *dir = "";
            return true;
        }

        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
        case RETRO_ENVIRONMENT_SET_GEOMETRY:
        case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
        case RETRO_ENVIRONMENT_SET_VARIABLES:
            return true;

        case RETRO_ENVIRONMENT_GET_VARIABLE:
            return false;

        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
            return false;

        default:
            return false;
    }
}

static void video_cb(const void *data, unsigned width, unsigned height, size_t pitch)
{
    char l1[32], l2[32], l3[32], l4[32];

    g_frame_count++;

    if ((g_frame_count % 30) == 0 || g_frame_count == 1) {
        snprintf(l1, sizeof(l1), "PAD=%04X", (unsigned)ps2_input_buttons());
        snprintf(l2, sizeof(l2), "%ux%u", width, height);
        snprintf(l3, sizeof(l3), "P=%u", (unsigned)pitch);
        snprintf(l4, sizeof(l4), "FMT=%d", g_pixel_format);
        ps2_video_set_debug(l1, l2, l3, l4);
    }

    if (g_pixel_format == RETRO_PIXEL_FORMAT_RGB565)
        ps2_video_present_rgb565(data, width, height, pitch);
}

static void audio_cb(int16_t left, int16_t right)
{
    (void)left;
    (void)right;
}

static size_t audio_batch_cb(const int16_t *data, size_t frames)
{
    (void)data;
    return frames;
}

static void input_poll_cb(void)
{
    g_input_tick++;
    ps2_input_poll();
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)port;
    (void)device;
    (void)index;

#if AUTO_INPUT_TEST
    if (id == RETRO_DEVICE_ID_JOYPAD_START) {
        if (auto_hold(g_input_tick, 120, 180))
            return 1;
        if (auto_hold(g_input_tick, 300, 360))
            return 1;
    }

    if (id == RETRO_DEVICE_ID_JOYPAD_A) {
        if (auto_hold(g_input_tick, 300, 360))
            return 1;
    }

    if (id == RETRO_DEVICE_ID_JOYPAD_RIGHT) {
        if (auto_hold(g_input_tick, 500, 620))
            return 1;
    }

    if (id == RETRO_DEVICE_ID_JOYPAD_B) {
        if (auto_hold(g_input_tick, 650, 720))
            return 1;
    }

    return 0;
#else
    return ps2_input_libretro_state(id);
#endif
}

int main(int argc, char *argv[])
{
    struct retro_system_info info;
    struct retro_game_info game;
    size_t rom_size;

    (void)argc;
    (void)argv;

    SifInitRpc(0);
    init_scr();

    scr_printf("ps2snes2005 embedded rom boot test\n");

    if (!ps2_video_init_once())
        die("ps2_video_init_once() falhou");

    if (ps2_input_init_once())
        scr_printf("input init ok\n");
    else
        scr_printf("input init falhou\n");

    retro_set_environment(environ_cb);
    retro_set_video_refresh(video_cb);
    retro_set_audio_sample(audio_cb);
    retro_set_audio_sample_batch(audio_batch_cb);
    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);

    retro_init();

    memset(&info, 0, sizeof(info));
    retro_get_system_info(&info);

    scr_printf("core: %s %s\n",
               info.library_name ? info.library_name : "(null)",
               info.library_version ? info.library_version : "(null)");
    scr_printf("need_fullpath=%d block_extract=%d valid_ext=%s\n",
               info.need_fullpath,
               info.block_extract,
               info.valid_extensions ? info.valid_extensions : "(null)");

    rom_size = (size_t)(smw_sfc_end - smw_sfc_start);

    memset(&game, 0, sizeof(game));
    game.path = "smw.sfc";
    game.data = smw_sfc_start;
    game.size = rom_size;
    game.meta = NULL;

    scr_printf("embedded rom size: %u bytes\n", (unsigned)rom_size);

    if (!retro_load_game(&game))
        die("retro_load_game() falhou");

    scr_printf("retro_load_game() OK\n");
    scr_printf("entrando no loop...\n");

    while (1) {
        retro_run();
    }

    return 0;
}
