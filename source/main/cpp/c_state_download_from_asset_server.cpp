#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "rwifi/c_tcp_client.h"
#include "rwifi/c_tcp_client_plugins.h"

#include "main/c_app_data.h"

namespace ncore
{
    void on_download_complete(void* on_complete_context, u32 data_type, u32 data_size, byte const* data_ptr)
    {
        app_data_t* app_data = (app_data_t*)on_complete_context;

        if (data_type == 0)
        {
            // Store the downloaded script binary in PSRAM
            app_data->m_script_binary      = (void*)data_ptr;
            app_data->m_script_binary_size = data_size;
        }
        else if (data_type == 1)
        {
            // Store the downloaded sprite pack in PSRAM
            app_data->m_sprites = (ngx2::sprite_pack_t*)data_ptr;
        }
        else if (data_type == 2)
        {
            // Store the downloaded font pack in PSRAM
            app_data->m_fonts = (ngx2::font_pack_t*)data_ptr;
        }
        else if (data_type == 3)
        {
            // Store the downloaded palette pack in PSRAM
            app_data->m_palettes = (ngx2::palette_pack_t*)data_ptr;
        }
    }

    static void on_handshake_complete(void* on_complete_context, u32 data_type, u32 data_size, byte const* data_ptr)
    {
        app_data_t* app_data = (app_data_t*)on_complete_context;

        if (data_type == 1)
        {
            // After the handshake is complete, we can start downloading the assets from the asset server.
            // We will use the download plugin to handle the downloading of the assets.
            app_data->m_state_data.m_state_data = ASSET_SERVER_STATE_DOWNLOADING;
        }
        else
        {
            // Error during handshake, we can set the state to error and handle it accordingly.
            app_data->m_state_data.m_state_data = ASSET_SERVER_STATE_ERROR;
        }
    }

    enum asset_server_state_t
    {
        ASSET_SERVER_STATE_SETUP = 0,
        ASSET_SERVER_STATE_CONNECTING,
        ASSET_SERVER_STATE_CONNECTED,
        ASSET_SERVER_STATE_DOWNLOADING,
        ASSET_SERVER_STATE_COMPLETE,
        ASSET_SERVER_STATE_ERROR,
    };

    void state_download_from_asset_server(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms)
    {
        if (state_data.m_state_data == ASSET_SERVER_STATE_SETUP)
        {
            void* tcp_socket = nnet::setup_default(&app_data.m_tcpclient_config);
            nnet::setup(app_data.m_tcpclient_asset_server, &app_data.m_tcpclient_config, tcp_socket, ASSET_SERVER_IP(), ASSET_SERVER_TCPPORT());

            // Create the tcp client plugins for handshake and downloading, and register them.
            // Also set our download complete callback
            nnet::tcp_recv_plugin_t* handshake_plugin = nnet::new_handshake_plugin(on_handshake_complete, &app_data);
            nnet::tcp_recv_plugin_t* download_plugin  = nnet::new_download_plugin(on_download_complete, &app_data);

            // Register the necessary plugins with the TCP client
            nnet::register_plugin(app_data.m_tcpclient_asset_server, handshake_plugin);
            nnet::register_plugin(app_data.m_tcpclient_asset_server, download_plugin);

            // Start the connection to the asset server
            nnet::connect(app_data.m_tcpclient_asset_server);
            state_data.m_state_data = ASSET_SERVER_STATE_CONNECTING;
        }
        else if (state_data.m_state_data == ASSET_SERVER_STATE_CONNECTING)
        {
            // Check if the TCP client is connected to the asset server
            if (nnet::is_connected(app_data.m_tcpclient_asset_server))
            {
                state_data.m_state_data = ASSET_SERVER_STATE_CONNECTED;
            }
        }
        else if (state_data.m_state_data == ASSET_SERVER_STATE_CONNECTED)
        {
            //
            // Handshake in progress ....
            //
            // When connected, a handshake will be initiated by the asset server, and the handshake plugin will handle it.
            // Once the handshake is complete, the on_handshake_complete callback will be called, which will set the state
            // to ASSET_SERVER_STATE_DOWNLOADING or ASSET_SERVER_STATE_ERROR based on the handshake result.
            //
        }
        else if (state_data.m_state_data == ASSET_SERVER_STATE_DOWNLOADING)
        {
            // Wait until we have received all the necessary data from the asset server:
            // - script binary
            // - sprite pack
            // - font pack
            // - palette pack
            //
            // Once we have received all the necessary data, we can move to the next state in the FSM.
            if (app_data.m_script_binary && app_data.m_sprites && app_data.m_fonts && app_data.m_palettes)
            {
                // Release the TCP client connection to the asset server, as we have completed the download.
                // Also free the plugins and their associated data, as they are no longer needed.

                // TODO
                // Possibly verify the integrity of the downloaded assets here (e.g., checksum, signature) before proceeding.

                // Leave
                state_data.m_next_state = (fsm_state_enum_t)(state_data.m_current_state + 1);  // Move to the next state in the FSM
            }
        }
    }

}  // namespace ncore