#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "main/c_app_data.h"

namespace ncore
{
    enum sensor_server_state_t
    {
        SENSOR_SERVER_STATE_SETUP = 0,
        SENSOR_SERVER_STATE_CONNECTING,
        SENSOR_SERVER_STATE_CONNECTED,
        SENSOR_SERVER_STATE_ERROR,
    };

    void state_connect_to_sensor_server(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms)
    {
        if (state_data.m_state_data == SENSOR_SERVER_STATE_SETUP)
        {
            void* tcp_socket = nnet::setup_default(&app_data.m_tcpclient_config);
            nnet::setup(app_data.m_tcpclient_sensor_server, &app_data.m_tcpclient_config, tcp_socket, SENSOR_SERVER_IP(), SENSOR_SERVER_TCPPORT());

            state_data.m_state_data = SENSOR_SERVER_STATE_CONNECTING;
        }
        else if (state_data.m_state_data == SENSOR_SERVER_STATE_CONNECTING)
        {
            // Check if the TCP client is connected to the sensor server
            if (nnet::is_connected(app_data.m_tcpclient_sensor_server))
            {
                state_data.m_state_data = SENSOR_SERVER_STATE_CONNECTED;
            }
        }
        else if (state_data.m_state_data == SENSOR_SERVER_STATE_CONNECTED)
        {
            to_state_next(state_data);  // Move to the next state in the FSM
        }
    }

}  // namespace ncore