#include "rcore/c_app.h"
#include "rcore/c_network.secret.h"
#include "rcore/c_log.h"

#include "main/c_app_data.h"

namespace ncore
{ 
    void state_connect_to_wifi(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms)
    {
        if (state_data.m_state_data == 0)
        {
            nnet::setup(app_data.m_wifi_manager, &app_data.m_wifi_config);
            nnet::activate(app_data.m_wifi_manager);
            state_data.m_state_data = 1;
        }
        else if (state_data.m_state_data == 1)
        {
            // Check WiFi connection status
            if (nnet::is_connected(app_data.m_wifi_manager))
            {
                state_data.m_state_data = 2;
            }
        }
        else if (state_data.m_state_data == 2)
        {
            // Leave
            to_state_next(state_data);
        }
    }
}  // namespace ncore