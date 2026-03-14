#include "launcher_pages.h"

#include <stdio.h>
#include <string.h>
#include "launcher_browser.h"
#include "launcher_font.h"
#include "launcher_logo.h"
#include "launcher_theme.h"

static void fit_text(char *out, size_t out_size, const char *src, int max_chars)
{
    size_t len;

    if (!out || out_size == 0)
        return;

    if (!src) {
        out[0] = '\0';
        return;
    }

    len = strlen(src);
    if ((int)len <= max_chars) {
        snprintf(out, out_size, "%s", src);
        return;
    }

    if (max_chars < 4) {
        out[0] = '\0';
        return;
    }

    snprintf(out, out_size, "...%s", src + (len - (size_t)(max_chars - 3)));
}

static void draw_main_page(const launcher_state_t *state)
{
    char label[96];

    fit_text(label, sizeof(label), state->selected_label, 44);

    launcher_logo_draw(LAUNCHER_LOGO_X, LAUNCHER_LOGO_Y);

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_ITEM0_X, LAUNCHER_MAIN_ITEM0_Y,
        state->main_sel == 0 ? "> START GAME" : "  START GAME",
        state->main_sel == 0 ? LAUNCHER_COLOR_HIGHLIGHT : LAUNCHER_COLOR_TEXT,
        3
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_ITEM1_X, LAUNCHER_MAIN_ITEM1_Y,
        state->main_sel == 1 ? "> BROWSE USB" : "  BROWSE USB",
        state->main_sel == 1 ? LAUNCHER_COLOR_HIGHLIGHT : LAUNCHER_COLOR_TEXT,
        3
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_ITEM2_X, LAUNCHER_MAIN_ITEM2_Y,
        state->main_sel == 2 ? "> OPTIONS" : "  OPTIONS",
        state->main_sel == 2 ? LAUNCHER_COLOR_HIGHLIGHT : LAUNCHER_COLOR_TEXT,
        3
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_INFO_X, LAUNCHER_MAIN_INFO_Y,
        "CURRENT FILE",
        LAUNCHER_COLOR_TEXT,
        2
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_VALUE_X, LAUNCHER_MAIN_VALUE_Y,
        label,
        LAUNCHER_COLOR_HIGHLIGHT,
        2
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_HINT1_X, LAUNCHER_HINT1_Y,
        "CROSS START = OPEN",
        LAUNCHER_COLOR_TEXT,
        2
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_HINT2_X, LAUNCHER_HINT2_Y,
        "UP DOWN = MOVE",
        LAUNCHER_COLOR_TEXT,
        2
    );
}

static void draw_browser_page(void)
{
    char path_line[96];
    int count = launcher_browser_count();
    int scroll = launcher_browser_scroll();
    int selected = launcher_browser_selected();
    int row;

    fit_text(path_line, sizeof(path_line), launcher_browser_current_path(), 48);

    launcher_font_draw_string_color_scaled(
        LAUNCHER_BROWSER_TITLE_X, LAUNCHER_BROWSER_TITLE_Y,
        "USB BROWSER",
        LAUNCHER_COLOR_TEXT,
        4
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_BROWSER_PATH_X, LAUNCHER_BROWSER_PATH_Y,
        path_line,
        LAUNCHER_COLOR_HIGHLIGHT,
        2
    );

    if (launcher_browser_last_error()) {
        launcher_font_draw_string_color_scaled(178, 184, "OPEN FAILED", LAUNCHER_COLOR_HIGHLIGHT, 3);
    } else if (count <= 0) {
        launcher_font_draw_string_color_scaled(96, 184, "NO FOLDERS OR ROMS", LAUNCHER_COLOR_HIGHLIGHT, 3);
    } else {
        for (row = 0; row < LAUNCHER_BROWSER_ROWS; row++) {
            int index = scroll + row;
            const launcher_browser_entry_t *entry;
            char line[96];
            char name_buf[72];

            if (index >= count)
                break;

            entry = launcher_browser_entry(index);
            if (!entry)
                break;

            fit_text(name_buf, sizeof(name_buf), entry->name, 40);

            if (entry->is_dir)
                snprintf(line, sizeof(line), "%s DIR %s", index == selected ? ">" : " ", name_buf);
            else
                snprintf(line, sizeof(line), "%s %s", index == selected ? ">" : " ", name_buf);

            launcher_font_draw_string_color_scaled(
                LAUNCHER_BROWSER_LIST_X,
                LAUNCHER_BROWSER_LIST_Y + row * LAUNCHER_BROWSER_STEP_Y,
                line,
                index == selected ? LAUNCHER_COLOR_HIGHLIGHT : LAUNCHER_COLOR_TEXT,
                2
            );
        }
    }

    launcher_font_draw_string_color_scaled(24, 390, "CROSS OPEN SELECT", LAUNCHER_COLOR_TEXT, 2);
    launcher_font_draw_string_color_scaled(24, 414, "CIRCLE BACK  SELECT REFRESH", LAUNCHER_COLOR_TEXT, 2);
}

static void draw_options_page(void)
{
    launcher_font_draw_string_color_scaled(
        LAUNCHER_OPTIONS_TITLE_X, LAUNCHER_OPTIONS_TITLE_Y,
        "OPTIONS",
        LAUNCHER_COLOR_TEXT,
        4
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_OPTIONS_INFO_X, LAUNCHER_OPTIONS_INFO_Y,
        "COMING SOON",
        LAUNCHER_COLOR_HIGHLIGHT,
        3
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_OPTIONS_BACK_X, LAUNCHER_OPTIONS_BACK_Y,
        "> BACK",
        LAUNCHER_COLOR_TEXT,
        3
    );

    launcher_font_draw_string_color_scaled(
        54, 404,
        "CROSS START CIRCLE = BACK",
        LAUNCHER_COLOR_TEXT,
        2
    );
}

void launcher_pages_draw(const launcher_state_t *state)
{
    if (!state)
        return;

    if (state->page == LAUNCHER_PAGE_MAIN)
        draw_main_page(state);
    else if (state->page == LAUNCHER_PAGE_BROWSER)
        draw_browser_page();
    else
        draw_options_page();
}
