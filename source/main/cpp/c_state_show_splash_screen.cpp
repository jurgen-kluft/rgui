#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "lib_guition/c_lcd.h"

#include "cgx2/c_types.h"

#include "main/c_app_data.h"

namespace ncore
{
    enum especial_screens_t
    {
        SPECIAL_SCREEN_SPLASH = 0,
        SPECIAL_SCREEN_LOGO   = 1,
        SPECIAL_SCREEN_RAIN   = 2,
    };

    void state_show_splash_screen(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        ngx2::sprite_t* splash_sprite = ngx2::get_sprite(app_data->m_sprite_pack, SPECIAL_SCREEN_SPLASH);  
        if (splash_sprite == nullptr)
        {
            to_state_next(state_data);  // Move to the next state in the FSM
            return;
        }

        nlcd::draw_sprite(0, 0, (u16 const*)splash_sprite->pixel_data.data(), splash_sprite->width, splash_sprite->height);

        to_state_next(state_data);  // Move to the next state in the FSM
    }

}  // namespace ncore