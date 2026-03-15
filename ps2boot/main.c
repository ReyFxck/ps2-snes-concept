#include "rom_loader/rom_loader.h"
#include <kernel.h>
#include <sifrpc.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libpad.h>

#include "libretro.h"
#include "ps2_video.h"
#include "ps2_input.h"
#include "ps2_menu.h"
#include "ui/launcher/launcher.h"

extern unsigned char smw_sfc_start[];
extern unsigned char smw_sfc_end[];

#define DEBUG_OVERLAY 0

static unsigned g_frame_count = 0;
static enum retro_pixel_format g_pixel_format = RETRO_PIXEL_FORMAT_RGB565;
static uint32_t g_prev_buttons = 0;
static unsigned g_fps_display = 0;
static unsigned g_fps_accum = 0;
static clock_t g_fps_last_clock = 0;
static double g_core_nominal_fps = 60.0;
static clock_t g_frame_deadline = 0;

static void *g_loaded_rom_data = NULL;
static size_t g_loaded_rom_size = 0;
static char g_loaded_rom_name[256];

static void die(const char *msg)
{
    scr_printf("\n[ERRO] %s\n", msg);
    SleepThread();
    while (1) {}
}

static double target_limit_fps(void)
{
    int mode = ps2_menu_frame_limit_mode();

    if (mode == PS2_FRAME_LIMIT_50)
        return 50.0;
    if (mode == PS2_FRAME_LIMIT_60)
        return 60.0;
    if (mode == PS2_FRAME_LIMIT_OFF)
        return 0.0;

    if (g_core_nominal_fps > 1.0)
        return g_core_nominal_fps;

    return 60.0;
}

static void throttle_frame_if_needed(void)
{
    double fps = target_limit_fps();
    double ticks_per_frame_d;
    clock_t ticks_per_frame;
    clock_t now;

    if (fps <= 0.0)
        return;

    ticks_per_frame_d = (double)CLOCKS_PER_SEC / fps;
    if (ticks_per_frame_d < 1.0)
        ticks_per_frame_d = 1.0;

    ticks_per_frame = (clock_t)(ticks_per_frame_d + 0.5);
    now = clock();

    if (g_frame_deadline == 0) {
        g_frame_deadline = now + ticks_per_frame;
        return;
    }

    if (now < g_frame_deadline) {
        while ((now = clock()) < g_frame_deadline) {}
    } else if ((now - g_frame_deadline) > (ticks_per_frame * 4)) {
        g_frame_deadline = now;
    }

    g_frame_deadline += ticks_per_frame;
}

static void update_fps_overlay(void)
{
    clock_t now = clock();
    double target = target_limit_fps();
    char l1[32];
    char l2[32];

    g_fps_accum++;

    if (g_fps_last_clock == 0)
        g_fps_last_clock = now;

    if ((now - g_fps_last_clock) >= CLOCKS_PER_SEC) {
        g_fps_display = (unsigned)(((double)g_fps_accum * (double)CLOCKS_PER_SEC) / (double)(now - g_fps_last_clock) + 0.5);
        g_fps_accum = 0;
        g_fps_last_clock = now;
    }

    if (ps2_menu_show_fps_enabled()) {
        if (target > 0.0)
            snprintf(l1, sizeof(l1), "FPS: %u/%.0f", g_fps_display, target);
        else
            snprintf(l1, sizeof(l1), "FPS: %u/OFF", g_fps_display);

        if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_AUTO)
            snprintf(l2, sizeof(l2), "LIMIT: AUTO");
        else if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_50)
            snprintf(l2, sizeof(l2), "LIMIT: 50");
        else if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_60)
            snprintf(l2, sizeof(l2), "LIMIT: 60");
        else
            snprintf(l2, sizeof(l2), "LIMIT: OFF");

        if (ps2_menu_fps_rainbow_enabled()) {
            char l2fx[64];
            snprintf(l2fx, sizeof(l2fx), "%.56s MORE FPS", l2);
            ps2_video_set_debug(l1, l2fx, "", "");
        } else {
            ps2_video_set_debug(l1, l2, "", "");
        }
    } else {
        ps2_video_set_debug("", "", "", "");
    }
}

static bool environ_cb(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        g_pixel_format = *(const enum retro_pixel_format *)data;
        scr_printf("[ENV] pixel format = %d\n", g_pixel_format);
        return true;

    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
        const char **dir = (const char **)data;
        *dir = "";
        return true;
    }

    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
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
#if DEBUG_OVERLAY
    char l1[32], l2[32], l3[32], l4[32];
#endif

    g_frame_count++;

#if DEBUG_OVERLAY
    if ((g_frame_count % 30) == 0 || g_frame_count == 1) {
        snprintf(l1, sizeof(l1), "PAD=%04X", (unsigned)ps2_input_buttons());
        snprintf(l2, sizeof(l2), "%ux%u", width, height);
        snprintf(l3, sizeof(l3), "P=%u", (unsigned)pitch);
        snprintf(l4, sizeof(l4), "FMT=%d", g_pixel_format);
        ps2_video_set_debug(l1, l2, l3, l4);
    }
#endif

    update_fps_overlay();

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
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)port;
    (void)device;
    (void)index;
    return ps2_input_libretro_state(id);
}

static __attribute__((unused)) int load_file_to_memory(const char *path, void **out_data, size_t *out_size)
{
    FILE *fp;
    long file_size;
    void *buf;
    size_t read_bytes;

    if (!path || !path[0] || !out_data || !out_size)
        return 0;

    fp = fopen(path, "rb");
    if (!fp) {
        scr_printf("fopen falhou: %s\n", path);
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return 0;
    }

    file_size = ftell(fp);
    if (file_size <= 0) {
        fclose(fp);
        return 0;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return 0;
    }

    buf = malloc((size_t)file_size);
    if (!buf) {
        fclose(fp);
        scr_printf("malloc falhou: %ld bytes\n", file_size);
        return 0;
    }

    read_bytes = fread(buf, 1, (size_t)file_size, fp);
    fclose(fp);

    if (read_bytes != (size_t)file_size) {
        free(buf);
        scr_printf("fread incompleto: %u/%u\n", (unsigned)read_bytes, (unsigned)file_size);
        return 0;
    }

    *out_data = buf;
    *out_size = (size_t)file_size;
    return 1;
}

static int load_selected_game(void)
{
    struct retro_game_info game;
    const char *path = launcher_selected_path();

    memset(&game, 0, sizeof(game));

    if (path && path[0]) {
        if (g_loaded_rom_data) {
            rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
                    g_loaded_rom_name[0] = '\0';
            g_loaded_rom_data = NULL;
            g_loaded_rom_size = 0;
        }

        if (!rom_loader_load(path, &g_loaded_rom_data, &g_loaded_rom_size, g_loaded_rom_name, sizeof(g_loaded_rom_name)))
            return 0;

        game.path = g_loaded_rom_name[0] ? g_loaded_rom_name : path;
        game.data = g_loaded_rom_data;
        game.size = g_loaded_rom_size;
        game.meta = NULL;

        scr_printf("rom externa: %s\n", path);
        scr_printf("rom size: %u bytes\n", (unsigned)g_loaded_rom_size);
        return retro_load_game(&game) ? 1 : 0;
    }

    g_loaded_rom_name[0] = '\0';
    game.path = "smw.sfc";
    game.data = smw_sfc_start;
    game.size = (size_t)(smw_sfc_end - smw_sfc_start);
    game.meta = NULL;

    scr_printf("embedded rom size: %u bytes\n", (unsigned)game.size);
    return retro_load_game(&game) ? 1 : 0;
}

static void run_launcher(void)
{
    g_prev_buttons = 0;
    launcher_init();

    while (!launcher_should_start_game()) {
        uint32_t buttons;
        uint32_t pressed;

        ps2_input_poll();
        buttons = ps2_input_buttons();
        pressed = buttons & ~g_prev_buttons;

        launcher_handle(pressed);
        launcher_draw();

        g_prev_buttons = buttons;
    }

    launcher_clear_start_request();
    g_prev_buttons = 0;
}

int main(int argc, char *argv[])
{
    struct retro_system_info info;
    struct retro_system_av_info av;
    int saved_launcher_x;
    int saved_launcher_y;

    (void)argc;
    (void)argv;

    SifInitRpc(0);
    init_scr();

    scr_printf("ps2snes2005 launcher boot\n");

    if (!ps2_video_init_once())
        die("ps2_video_init_once() falhou");

    if (ps2_input_init_once())
        scr_printf("input init ok\n");
    else
        scr_printf("input init falhou\n");

    ps2_menu_init();

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

    ps2_video_get_offsets(&saved_launcher_x, &saved_launcher_y);
    ps2_video_set_offsets(0, 0);
    scr_clear();
    run_launcher();
    ps2_video_set_offsets(saved_launcher_x, saved_launcher_y);

    if (!load_selected_game())
        die("retro_load_game() falhou");

    memset(&av, 0, sizeof(av));
    retro_get_system_av_info(&av);
    if (av.timing.fps > 1.0)
        g_core_nominal_fps = av.timing.fps;

    scr_printf("retro_load_game() OK\n");
    scr_printf("selected: %s\n", launcher_selected_label());
    scr_printf("core nominal fps: %.3f\n", g_core_nominal_fps);
    scr_printf("entrando no loop...\n");

    scr_clear();

    while (1) {
        uint32_t buttons;
        uint32_t pressed;

        ps2_input_poll();
        buttons = ps2_input_buttons();
        pressed = buttons & ~g_prev_buttons;

        if (ps2_menu_is_open()) {
            ps2_menu_handle(pressed);

            if (ps2_menu_restart_game_requested()) {
                ps2_menu_clear_restart_game_request();
                ps2_menu_close();
                retro_unload_game();
                if (g_loaded_rom_data) {
                    rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
                    g_loaded_rom_name[0] = '\0';
                    g_loaded_rom_data = NULL;
                    g_loaded_rom_size = 0;
                }
                ps2_video_set_debug("", "", "", "");
                g_prev_buttons = 0;
                g_frame_deadline = 0;
                g_fps_display = 0;
                g_fps_accum = 0;
                g_fps_last_clock = 0;
                g_core_nominal_fps = 60.0;
                scr_clear();
                if (!load_selected_game())
                    die("retro_load_game() falhou");
                memset(&av, 0, sizeof(av));
                retro_get_system_av_info(&av);
                if (av.timing.fps > 1.0)
                    g_core_nominal_fps = av.timing.fps;
                scr_clear();
                continue;
            }

            if (ps2_menu_exit_game_requested()) {
                ps2_menu_clear_exit_game_request();
                ps2_menu_close();
                retro_unload_game();
                if (g_loaded_rom_data) {
                    rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
                    g_loaded_rom_name[0] = '\0';
                    g_loaded_rom_data = NULL;
                    g_loaded_rom_size = 0;
                }
                ps2_video_set_debug("", "", "", "");
                g_prev_buttons = 0;
                g_frame_deadline = 0;
                g_fps_display = 0;
                g_fps_accum = 0;
                g_fps_last_clock = 0;
                g_core_nominal_fps = 60.0;
                ps2_video_get_offsets(&saved_launcher_x, &saved_launcher_y);
                ps2_video_set_offsets(0, 0);
                scr_clear();
                run_launcher();
                ps2_video_set_offsets(saved_launcher_x, saved_launcher_y);
                if (!load_selected_game())
                    die("retro_load_game() falhou");
                memset(&av, 0, sizeof(av));
                retro_get_system_av_info(&av);
                if (av.timing.fps > 1.0)
                    g_core_nominal_fps = av.timing.fps;
                scr_clear();
                continue;
            }

            if (ps2_menu_is_open())
                ps2_menu_draw();
            g_prev_buttons = buttons;
            continue;
        }

        if (pressed & PAD_SELECT) {
            ps2_menu_open();
            ps2_menu_draw();
            g_prev_buttons = buttons;
            continue;
        }

        retro_run();
        throttle_frame_if_needed();
        g_prev_buttons = buttons;
    }

    return 0;
}
