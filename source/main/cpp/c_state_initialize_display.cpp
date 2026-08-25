#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "lib_guition/c_lcd.h"
#include "gui/c_display.h"

#include "main/c_main.h"

namespace ncore
{ 
    i32 state_initialize_display(state_call_t call, app_data_t& app_data)
    {
        if (call == STATE_CALL_ENTER)
        {
            nlog::println("Initialize Display - Begin");

            if (nlcd::initialize() == false)
            {
                nlog::println("Initialize Display - Failed to initialize LCD");
                return -1;
            }

            nlog::println("Initialize Display - Complete");
        }
        return 1;
    }

}  // namespace ncore