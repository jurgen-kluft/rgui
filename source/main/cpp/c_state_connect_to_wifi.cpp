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

    void state_connect_to_wifi(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_state_data == WIFI_STATE_INIT)
        {
            // Note: WiFi configuration is initialized at 'c_app_data.cpp'

            nnet::setup(app_data->m_wifi_manager, &app_data->m_wifi_config);
            nnet::activate(app_data->m_wifi_manager);
            state_data->m_state_data = WIFI_STATE_CONNECTING;
        }
        else if (state_data->m_state_data == WIFI_STATE_CONNECTING)
        {
            // Check WiFi connection status
            if (nnet::is_connected(app_data->m_wifi_manager))
            {
                state_data->m_state_data = WIFI_STATE_CONNECTED;
            }
        }
        else if (state_data->m_state_data == WIFI_STATE_CONNECTED)
        {
            // Leave
            to_state_next(state_data);
        }
    }
}  // namespace ncore