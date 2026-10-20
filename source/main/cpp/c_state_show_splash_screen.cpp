#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "gui/c_draw.h"
#include "cgx2/c_types.h"

#include "main/c_app_data.h"

namespace ncore
{
    enum esplash_screen_t
    {
        SPECIAL_SCREEN_SPLASH = 0,
        SPECIAL_SCREEN_LOGO   = 1,
        SPECIAL_SCREEN_RAIN   = 2
    };

    enum esplash_state_t
    {
        SPLASH_STATE_INIT = 0,
        SPLASH_STATE_SHOW,
        SPLASH_STATE_FADE_OUT,
        SPLASH_STATE_DONE
    };

    static u64       s_splash_start_time  = 0;
    static const u64 s_splash_duration_ms = 3000;  // Duration to show the splash screen in milliseconds

    void state_show_splash_screen(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_state_data == SPLASH_STATE_INIT)
        {
            s_splash_start_time = now_ms;
            state_data->m_state_data = SPLASH_STATE_SHOW;
        }
        else if (state_data->m_state_data == SPLASH_STATE_FADE_OUT)
        {
            // Handle fade out logic here if needed
            if (now_ms - s_splash_start_time >= s_splash_duration_ms)
            {
                state_data->m_state_data = SPLASH_STATE_DONE;
                to_state_next(state_data);  // Move to the next state in the FSM
            }
            else
            {
                const u8 fade_progress = (u8)(((now_ms - s_splash_start_time) * 255) / s_splash_duration_ms);  // Calculate fade progress (0-255)
                ngui::draw_begin_frame();
                {
                    ngui::draw_background(SPECIAL_SCREEN_SPLASH, 255 - fade_progress);
                }
                ngui::draw_end_frame();
            }
        }
        else if (state_data->m_state_data == SPLASH_STATE_SHOW)
        {
            ngui::draw_begin_frame();
            {
                ngui::draw_background(SPECIAL_SCREEN_SPLASH, 255);
            }
            ngui::draw_end_frame();

            state_data->m_state_data = SPLASH_STATE_FADE_OUT;
        }
    }

}  // namespace ncore