#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "main/c_app_data.h"

namespace ncore
{
    void state_active_display_on(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms)
    {
        // if (ntimer::millis() - toggle_lcd_fill_time > 2000)
        // {
        //     nlog::println("Filling random area with random color");

        //     // Fill a random area with a random color every 2 seconds
        //     u16 color = (u16)g_random_u32(&gAppState.m_random, 16);  // Random RGB565 color
        //     u16 sx    = (u16)g_random_u32_range(&gAppState.m_random, 0, nlcd::width() - 51);
        //     u16 sy    = (u16)g_random_u32_range(&gAppState.m_random, 0, nlcd::height() - 51);
        //     u16 ex    = sx + (g_random_u32_range(&gAppState.m_random, 0, 40) + 10);  // Random width between 10 and 50 pixels
        //     u16 ey    = sy + (g_random_u32_range(&gAppState.m_random, 0, 40) + 10);  // Random height between 10 and 50 pixels

        //     nlcd::draw_rectangle(sx, sy, ex, ey, color);

        //     toggle_lcd_fill_time = ntimer::millis();
        // }

    }

}  // namespace ncore