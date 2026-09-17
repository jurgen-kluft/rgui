#include "rcore/c_app.h"
#include "rcore/c_log.h"
#include "rcore/c_system.h"

#include "ccore/c_memory.h"

#include "ccova/linked_program.h"

#include "rwifi/c_tcp_client.h"
#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client_plugins.h"

#include "rhome/c_msg.h"

#include "main/c_app_data.h"

namespace ncore
{
    void on_download_begin(void* user_ctx, u32 data_type, u32 data_size, nnet::buffer_t& buffer)
    {
        buffer.m_buffer = nullptr;
        buffer.m_length = 0;

        if (data_type == DATA_TYPE_SCRIPT_BINARY)
        {
            // Store the downloaded script binary in SRAM
            buffer.m_buffer = (byte*)nsystem::malloc(data_size);
            buffer.m_length = data_size;
        }
        else if (data_type == DATA_TYPE_SPRITE_PACK)
        {
            // Store the downloaded sprite pack in PSRAM
            buffer.m_buffer = nsystem::alloc_psram_aligned(data_size, 32);
            buffer.m_length = data_size;
        }
        else if (data_type == DATA_TYPE_FONT_PACK)
        {
            // Store the downloaded font pack in PSRAM
            buffer.m_buffer = nsystem::alloc_psram_aligned(data_size, 32);
            buffer.m_length = data_size;
        }
        else if (data_type == DATA_TYPE_PALETTE_PACK)
        {
            // Store the downloaded palette pack in PSRAM
            buffer.m_buffer = nsystem::alloc_psram_aligned(data_size, 32);
            buffer.m_length = data_size;
        }
    }

    void on_download_complete(void* user_ctx, u32 data_type, nnet::buffer_t buffer)
    {
        app_data_t* app_data = (app_data_t*)user_ctx;

        if (data_type == DATA_TYPE_SCRIPT_BINARY)
        {
            // Store the downloaded script binary in PSRAM
            app_data->m_script_binary      = (linked_program_t*)buffer.m_buffer;
            app_data->m_script_binary_size = buffer.m_length;
        }
        else if (data_type == DATA_TYPE_SPRITE_PACK)
        {
            // Store the downloaded sprite pack in PSRAM
            app_data->m_sprite_pack = (ngx2::sprite_pack_t*)buffer.m_buffer;
        }
        else if (data_type == DATA_TYPE_FONT_PACK)
        {
            // Store the downloaded font pack in PSRAM
            app_data->m_font_pack = (ngx2::font_pack_t*)buffer.m_buffer;
        }
        else if (data_type == DATA_TYPE_PALETTE_PACK)
        {
            // Store the downloaded palette pack in PSRAM
            app_data->m_palette_pack = (ngx2::palette_pack_t*)buffer.m_buffer;
        }
    }

    static void on_download_abort(void* on_abort_context, u32 data_type, nnet::buffer_t buffer)
    {
        if (buffer.m_buffer)
        {
            nsystem::free(buffer.m_buffer);
        }
    }

#define MSG_TYPE_ASSET_SERVER_REQUEST 0x20

    struct asset_server_request_t : public nnet::msg_hdr_t
    {
        // pairs of "u32 asset_type" and "u32 asset_version" follow
    };

    enum asset_server_state_t
    {
        ASSET_SERVER_STATE_SETUP = 0,
        ASSET_SERVER_STATE_CONNECTING,
        ASSET_SERVER_STATE_CONNECTED,
        ASSET_SERVER_STATE_DOWNLOADING,
        ASSET_SERVER_STATE_COMPLETE,
        ASSET_SERVER_STATE_ERROR,
    };

    void state_download_assets(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_state_data == ASSET_SERVER_STATE_SETUP)
        {
            app_data->m_tcp_socket = nnet::setup_default(&app_data->m_tcp_client_config);
            nnet::setup(app_data->m_tcp_client, &app_data->m_tcp_client_config, app_data->m_tcp_socket);

            // Create the tcp client plugins for handshake and downloading, and register them.
            // Also set our download complete callback
            nnet::tcp_recv_plugin_t* download_plugin = nnet::new_download_plugin(on_download_begin, on_download_complete, on_download_abort, &app_data);

            // Register the necessary plugins with the TCP client
            nnet::register_plugin(app_data->m_tcp_client, 0, download_plugin);

            // Start the connection to the asset server
            nnet::connect(app_data->m_tcp_client, ASSET_SERVER_IP(), ASSET_SERVER_TCPPORT());
            state_data->m_state_data = ASSET_SERVER_STATE_CONNECTING;
        }
        else if (state_data->m_state_data == ASSET_SERVER_STATE_CONNECTING)
        {
            // Check if the TCP client is connected to the asset server
            if (nnet::is_connected(app_data->m_tcp_client))
            {
                state_data->m_state_data = ASSET_SERVER_STATE_CONNECTED;
            }
        }
        else if (state_data->m_state_data == ASSET_SERVER_STATE_CONNECTED)
        {
            //
            // Handshake in progress ....
            //
            // When connected, a handshake will be initiated by the asset server, and the handshake plugin will handle it.
            // Once the handshake is complete, the on_handshake_complete callback will be called, which will set the state
            // to ASSET_SERVER_STATE_DOWNLOADING or ASSET_SERVER_STATE_ERROR based on the handshake result.
            //
        }
        else if (state_data->m_state_data == ASSET_SERVER_STATE_DOWNLOADING)
        {
            // Wait until we have received all the necessary data from the asset server:
            // - script binary
            // - sprite pack
            // - font pack
            // - palette pack
            //
            // Once we have received all the necessary data, we can move to the next state in the FSM.
            if (app_data->m_script_binary && app_data->m_sprite_pack && app_data->m_font_pack && app_data->m_palette_pack)
            {
                // Release the TCP client connection to the asset server, as we have completed the download.
                // Also free the plugins and their associated data, as they are no longer needed.

                // TODO
                // Possibly verify the integrity of the downloaded assets here (e.g., checksum, signature) before proceeding.
                nnet::tcp_recv_plugin_t* download_plugin  = nnet::get_plugin(app_data->m_tcp_client, 0);
                nnet::unregister_plugin(app_data->m_tcp_client, 0, download_plugin);

                nnet::disconnect(app_data->m_tcp_client);

                // Leave
                to_state_next(state_data);
            }
        }
    }

}  // namespace ncore