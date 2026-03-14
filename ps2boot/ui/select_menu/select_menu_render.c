#include "select_menu_render.h"

#include "../../ps2_video.h"
#include "select_menu_pages.h"

void select_menu_render(const select_menu_state_t *state)
{
    if (!state || !state->open)
        return;

    ps2_video_ui_set_menu_target();
    ps2_video_menu_begin_frame();
    select_menu_pages_draw(state);
    ps2_video_menu_end_frame();
}
