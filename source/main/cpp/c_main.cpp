#include "rcore/c_app.h"
#include "rcore/c_timer.h"
#include "rcore/c_log.h"
#include "rcore/c_system.h"

#include "main/c_app_data.h"

namespace ncore
{
    static app_data_t gAppState;

    namespace napp
    {
        void wakeup(state_t* state, ncore::nwakeup::reason_t reason)
        {
            // Handle wakeup reasons if needed (e.g., from deep sleep)
        }

        void presetup(state_t* state)
        {
            // Time critical setup before WiFi and other components are initialized can be done here
        }

        ntimer::periodic_task_t gBlinkLedTask = {1000, 0, nullptr, [](void* user) { nlcd::led_toggle(); }};

        void setup(state_t* state)
        {
            // Initialize application data
            init_app_data(gAppState);
        }

        void tick(state_t* state)
        {
            const u64 now_ms = ntimer::millis();

            // State Machine handling
            if (gAppState.m_state_data.m_current_state > FSM_STATE_NONE && gAppState.m_state_data.m_current_state < FSM_STATE_COUNT)
            {
                state_fn_t state_fn = gAppState.m_state_fn[gAppState.m_state_data.m_current_state];
                if (state_fn)
                {
                    state_fn(gAppState.m_state_data, gAppState, now_ms);
                    handle_state_data(gAppState.m_state_data, gAppState);
                }
            }
            else
            {
                // error 
                return;
            }
            
        }

    }  // namespace napp
}  // namespace ncore