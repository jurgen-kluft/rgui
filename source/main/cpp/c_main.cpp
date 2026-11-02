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

        void setup(state_t* state)
        {
            // Initialize application data
            init_app_data(&gAppState);
        }

        void tick(state_t* state)
        {
            const u64 now_ms = ntimer::millis();

            update_tick_fns(&gAppState);
            update_fsm(&gAppState, now_ms);
        }

    }  // namespace napp
}  // namespace ncore