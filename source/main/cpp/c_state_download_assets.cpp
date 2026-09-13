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
    enum edata_type_t
    {
        DATA_TYPE_SCRIPT_BINARY = 0,
        DATA_TYPE_SPRITE_PACK   = 1,
        DATA_TYPE_FONT_PACK     = 2,
        DATA_TYPE_PALETTE_PACK  = 3,
    };

    void on_download_begin(void* user_ctx, u32 data_type, u32 data_size, byte*& data_ptr)
    {
        data_ptr = nullptr;

        if (data_type == DATA_TYPE_SCRIPT_BINARY)
        {
            // Store the downloaded script binary in SRAM
            data_ptr = (byte*)nsystem::malloc(data_size);
        }
        else if (data_type == DATA_TYPE_SPRITE_PACK)
        {
            // Store the downloaded sprite pack in PSRAM
            data_ptr = nsystem::alloc_psram_aligned(data_size, 32);
        }
        else if (data_type == DATA_TYPE_FONT_PACK)
        {
            // Store the downloaded font pack in PSRAM
            data_ptr = nsystem::alloc_psram_aligned(data_size, 32);
        }
        else if (data_type == DATA_TYPE_PALETTE_PACK)
        {
            // Store the downloaded palette pack in PSRAM
            data_ptr = nsystem::alloc_psram_aligned(data_size, 32);
        }
    }

    void on_download_complete(void* user_ctx, u32 data_type, u32 data_size, byte const* data_ptr)
    {
        app_data_t* app_data = (app_data_t*)user_ctx;

        if (data_type == DATA_TYPE_SCRIPT_BINARY)
        {
            // Store the downloaded script binary in PSRAM
            app_data->m_script_binary      = (linked_program_t*)data_ptr;
            app_data->m_script_binary_size = data_size;
        }
        else if (data_type == DATA_TYPE_SPRITE_PACK)
        {
            // Store the downloaded sprite pack in PSRAM
            app_data->m_sprite_pack = (ngx2::sprite_pack_t*)data_ptr;
        }
        else if (data_type == DATA_TYPE_FONT_PACK)
        {
            // Store the downloaded font pack in PSRAM
            app_data->m_font_pack = (ngx2::font_pack_t*)data_ptr;
        }
        else if (data_type == DATA_TYPE_PALETTE_PACK)
        {
            // Store the downloaded palette pack in PSRAM
            app_data->m_palette_pack = (ngx2::palette_pack_t*)data_ptr;
        }
    }

    enum easset_type
    {
        DATA_TYPE_SCRIPT_BINARY = 0,
        DATA_TYPE_SPRITE_PACK,
        DATA_TYPE_FONT_PACK,
        DATA_TYPE_PALETTE_PACK,
    };

#define MSG_TYPE_ASSET_SERVER_REQUEST 0x20

    struct asset_server_request_t : public nnet::msg_hdr_t
    {
        // pairs of "u32 asset_type" and "u32 asset_version" follow
    };

    static void on_handshake_complete(void* user_ctx, u32 data_type, u32 data_size, byte const* data_ptr)
    {
        app_data_t* app_data = (app_data_t*)user_ctx;

        if (data_type == 1)  // 1 means handshake success
        {
            byte msg_memory[64];

            asset_server_request_t* assets_msg = (asset_server_request_t*)msg_memory;
            assets_msg->Magic                  = 0xF00D;
            assets_msg->Type                   = MSG_TYPE_ASSET_SERVER_REQUEST;
            assets_msg->PayloadSize            = 4 * (4 + 4);
            assets_msg->Checksum               = 0;  // No checksum

            const u8* mac = nnet::get_mac_address(app_data->m_wifi_manager);
            g_memcpy(assets_msg->Mac, mac, 6);

            const u32 script_version       = (app_data->m_script_binary != nullptr) ? app_data->m_script_binary->m_version : 0;
            const u32 sprite_pack_version  = (app_data->m_sprite_pack != nullptr) ? app_data->m_sprite_pack->m_version : 0;
            const u32 font_pack_version    = (app_data->m_font_pack != nullptr) ? app_data->m_font_pack->m_version : 0;
            const u32 palette_pack_version = (app_data->m_palette_pack != nullptr) ? app_data->m_palette_pack->m_version : 0;

            u32* payload = (u32*)(assets_msg + 1);
            payload[0]   = DATA_TYPE_SCRIPT_BINARY;
            payload[1]   = script_version;
            payload[2]   = DATA_TYPE_SPRITE_PACK;
            payload[3]   = sprite_pack_version;
            payload[4]   = DATA_TYPE_FONT_PACK;
            payload[5]   = font_pack_version;
            payload[6]   = DATA_TYPE_PALETTE_PACK;
            payload[7]   = palette_pack_version;

            nnet::send_later(app_data->m_tcpclient_asset_server, (byte*)assets_msg, sizeof(asset_server_request_t) + assets_msg->PayloadSize);

            // After the handshake is complete, and we have sent our asset request, we 
            // start downloading assets from the asset server.
            // We will use the download plugin to handle the downloading of the assets.
            // An asset that is already up-to-date as a message will contain no payload but
            // enough information to determine that it is up-to-date.
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

    void state_download_assets(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_state_data == ASSET_SERVER_STATE_SETUP)
        {
            void* tcp_socket = nnet::setup_default(&app_data->m_tcpclient_config);
            nnet::setup(app_data->m_tcpclient_asset_server, &app_data->m_tcpclient_config, tcp_socket, ASSET_SERVER_IP(), ASSET_SERVER_TCPPORT());

            // Create the tcp client plugins for handshake and downloading, and register them.
            // Also set our download complete callback
            nnet::tcp_recv_plugin_t* handshake_plugin = nnet::new_handshake_plugin(on_handshake_complete, &app_data);
            nnet::tcp_recv_plugin_t* download_plugin  = nnet::new_download_plugin(on_download_begin, on_download_complete, &app_data);

            // Register the necessary plugins with the TCP client
            nnet::register_plugin(app_data->m_tcpclient_asset_server, handshake_plugin);
            nnet::register_plugin(app_data->m_tcpclient_asset_server, download_plugin);

            // Start the connection to the asset server
            nnet::connect(app_data->m_tcpclient_asset_server);
            state_data->m_state_data = ASSET_SERVER_STATE_CONNECTING;
        }
        else if (state_data->m_state_data == ASSET_SERVER_STATE_CONNECTING)
        {
            // Check if the TCP client is connected to the asset server
            if (nnet::is_connected(app_data->m_tcpclient_asset_server))
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

                // Leave
                to_state_next(state_data);
            }
        }
    }

}  // namespace ncore