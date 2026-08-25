#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "main/c_app_data.h"

namespace ncore
{ 
    void state_connect_to_asset_server(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms)
    {
        if (state_data.m_state_data == 0)
        {
            void* tcp_socket = nnet::setup_default(&app_data.m_tcpclient_config);
            nnet::setup(app_data.m_tcpclient_asset_server, &app_data.m_tcpclient_config, tcp_socket, ASSET_SERVER_IP(), ASSET_SERVER_TCPPORT());
            state_data.m_state_data = 1;
        }
        else if (state_data.m_state_data == 1)
        {
            // Check if the TCP client is connected to the asset server
            if (nnet::is_connected(app_data.m_tcpclient_asset_server))
            {
                // Create the tcp client plugins for handshake and downloading, and register them.
                // Also set our download complete callback 

                state_data.m_state_data = 2;
            }
        }
        else if (state_data.m_state_data == 2)
        {
            // Leave
            state_data.m_next_state = (fsm_state_enum_t)(state_data.m_current_state + 1);  // Move to the next state in the FSM
        }

    }

}  // namespace ncore