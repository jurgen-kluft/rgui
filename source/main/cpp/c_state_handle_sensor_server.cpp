#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "rwifi/c_tcp_client.h"
#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client_plugins.h"

#include "rhome/c_msg.h"

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

    void on_message_begin(void* user_ctx, u32 data_type, u32 data_size, byte*& data_ptr)
    {
        data_ptr = nullptr;

    }

    void on_message_complete(void* user_ctx, u32 data_type, u32 data_size, byte const* data_ptr)
    {
        app_data_t* app_data = (app_data_t*)user_ctx;

    }

    void state_handle_sensor_server(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_state_data == SENSOR_SERVER_STATE_SETUP)
        {
            nnet::tcp_recv_plugin_t* messages_plugin = nnet::new_messages_plugin(on_message_begin, on_message_complete, &app_data);

            nnet::register_plugin(app_data->m_tcp_client, 0, messages_plugin);
            nnet::connect(app_data->m_tcp_client, SENSOR_SERVER_IP(), SENSOR_SERVER_TCPPORT());

            state_data->m_state_data = SENSOR_SERVER_STATE_CONNECTING;
        }
        else if (state_data->m_state_data == SENSOR_SERVER_STATE_CONNECTING)
        {
            // Check if the TCP client is connected to the sensor server
            if (nnet::is_connected(app_data->m_tcp_client))
            {
                state_data->m_state_data = SENSOR_SERVER_STATE_CONNECTED;
            }
        }
        else if (state_data->m_state_data == SENSOR_SERVER_STATE_CONNECTED)
        {
            to_state_next(state_data);  // Move to the next state in the FSM
        }
    }

}  // namespace ncore