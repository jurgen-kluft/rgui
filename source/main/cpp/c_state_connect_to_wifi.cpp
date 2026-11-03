#include "rcore/c_app.h"
#include "rcore/c_network.secret.h"
#include "rcore/c_log.h"

#include "main/c_app_data.h"

namespace ncore
{
    enum ewifi_state_t
    {
        WIFI_STATE_INIT       = 0,
        WIFI_STATE_CONNECTING = 1,
        WIFI_STATE_CONNECTED  = 2
    };

    static void s_tick_wifi_manager(app_data_t* app_data, u64 now_ms)
    {
        // tick the WiFi manager to handle any ongoing connection attempts or network events
        nnet::tick(app_data->m_wifi_manager);
    }

    void state_connect_to_wifi(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_state_data == WIFI_STATE_INIT)
        {
            // Note: WiFi manager is created in 'c_app_data.cpp'
            nnet::activate(app_data->m_wifi_manager);
            register_tick_fn(app_data, s_tick_wifi_manager);

            state_data->m_state_data     = WIFI_STATE_CONNECTING;
            state_data->m_state_start_ms = now_ms;
        }
        else if (state_data->m_state_data == WIFI_STATE_CONNECTING)
        {
            // Check WiFi connection status
            if (nnet::is_connected(app_data->m_wifi_manager))
            {
                state_data->m_state_data = WIFI_STATE_CONNECTED;
            }
            else if (state_data->m_state_start_ms + 30000 < now_ms)  // 30 seconds timeout
            {
                nlog::println("WiFi connection timed out");
                to_state_error(state_data);
            }
        }
        else if (state_data->m_state_data == WIFI_STATE_CONNECTED)
        {
            to_state_next(state_data);
        }
    }
}  // namespace ncore