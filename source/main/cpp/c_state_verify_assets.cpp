#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "lib_guition/c_sdcard.h"

#include ""

#include "main/c_app_data.h"

namespace ncore
{ 
    void state_handle_assets(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_current_state == 0)
        {
            nlog::log_info("VERIFY", "Begin");

            // Verify sprite, font and palette packs

            // Verify script binary

            // If all assets are verified successfully, then save all of
            // them to the appropriate storage (e.g., SD card and/or LittleFS)
            if (app_data->m_sd_card_available)
            {

            }

            // Leave
            to_state_next(state_data);
            nlog::log_info("VERIFY", "Ok");
        }        
    }

}  // namespace ncore