#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "lib_guition/c_lcd.h"
#include "gui/c_display.h"

#include "main/c_app_data.h"

namespace ncore
{
    void state_initialize_display(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms)
    {
        if (state_data.m_current_state == 0)
        {
            nlog::println("Initialize Display - Begin");

            if (nlcd::initialize() == false)
            {
                nlog::println("Initialize Display - Failed to initialize LCD");
                state_data.m_next_state = FSM_STATE_ERROR;
            }
            else
            {
                // Leave
                state_data.m_next_state = (fsm_state_enum_t)(state_data.m_current_state + 1);  // Move to the next state in the FSM
            }

            nlog::println("Initialize Display - Complete");
        }
    }

}  // namespace ncore