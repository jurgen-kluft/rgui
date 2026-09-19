#include "rcore/c_app.h"
#include "rcore/c_log.h"
#include "rcore/c_system.h"

#include "ccore/c_memory.h"

#include "ccova/linked_program.h"

#include "rwifi/c_tcp_client.h"
#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client_plugins.h"

#include "main/c_app_data.h"

namespace ncore
{
    // 8888888b.   .d88888b.  888       888 888b    888 888      .d88888b.        d8888 8888888b.
    // 888  "Y88b d88P" "Y88b 888   o   888 8888b   888 888     d88P" "Y88b      d88888 888  "Y88b
    // 888    888 888     888 888  d8b  888 88888b  888 888     888     888     d88P888 888    888
    // 888    888 888     888 888 d888b 888 888Y88b 888 888     888     888    d88P 888 888    888
    // 888    888 888     888 888d88888b888 888 Y88b888 888     888     888   d88P  888 888    888
    // 888    888 888     888 88888P Y88888 888  Y88888 888     888     888  d88P   888 888    888
    // 888  .d88P Y88b. .d88P 8888P   Y8888 888   Y8888 888     Y88b. .d88P d8888888888 888  .d88P
    // 8888888P"   "Y88888P"  888P     Y888 888    Y888 88888888 "Y88888P" d88P     888 8888888P"

    static bool on_download_begin(void* user_ctx, u32 data_type, u32 data_size, nnet::buffer_t& buffer)
    {
        app_data_t* app_data = (app_data_t*)user_ctx;

        switch (data_type)
        {
            case DATA_TYPE_SCRIPT_BINARY:
                // Store the downloaded script binary in SRAM
                if (data_size <= app_data->m_script_binary_capacity)
                {
                    buffer.m_buffer = (u8*)app_data->m_script_binary;
                    buffer.m_length = data_size;
                    return true;
                }
                break;
            case DATA_TYPE_SPRITE_PACK:
                // Store the downloaded sprite pack in PSRAM
                if (data_size <= app_data->m_sprite_pack_capacity)
                {
                    buffer.m_buffer = (u8*)app_data->m_sprite_pack;
                    buffer.m_length = data_size;
                    return true;
                }
                break;
            case DATA_TYPE_FONT_PACK:
                // Store the downloaded font pack in PSRAM
                if (data_size <= app_data->m_font_pack_capacity)
                {
                    buffer.m_buffer = (u8*)app_data->m_font_pack;
                    buffer.m_length = data_size;
                    return true;
                }
                break;
            case DATA_TYPE_PALETTE_PACK:
                // Store the downloaded palette pack in PSRAM
                if (data_size <= app_data->m_palette_pack_capacity)
                {
                    buffer.m_buffer = (u8*)app_data->m_palette_pack;
                    buffer.m_length = data_size;
                    return true;
                }
                break;
            case DATA_TYPE_HOUSE_META:
                // Store the downloaded house meta in PSRAM
                if (data_size <= app_data->m_house_meta_capacity)
                {
                    buffer.m_buffer = (u8*)app_data->m_house_meta;
                    buffer.m_length = data_size;
                    return true;
                }
                break;
        }

        buffer.m_buffer = nullptr;
        buffer.m_length = 0;
        return false;
    }

    static void on_download_complete(void* user_ctx, u32 data_type, nnet::buffer_t buffer)
    {
        app_data_t* app_data = (app_data_t*)user_ctx;
        switch (data_type)
        {
            case DATA_TYPE_SCRIPT_BINARY:
                app_data->m_script_binary_size = buffer.m_length;
                break;
            case DATA_TYPE_SPRITE_PACK:
                app_data->m_sprite_pack_size = buffer.m_length;
                break;
            case DATA_TYPE_FONT_PACK:
                app_data->m_font_pack_size = buffer.m_length;
                break;
            case DATA_TYPE_PALETTE_PACK:
                app_data->m_palette_pack_size = buffer.m_length;
                break;
            case DATA_TYPE_HOUSE_META:
                app_data->m_house_meta_size = buffer.m_length;
                break;
        }
    }

    static void on_download_abort(void* on_abort_context, u32 data_type, nnet::buffer_t buffer)
    {
        // nop
    }

    // 888b     d888 8888888888 .d8888b.   .d8888b.        d8888  .d8888b.  8888888888 .d8888b.
    // 8888b   d8888 888       d88P  Y88b d88P  Y88b      d88888 d88P  Y88b 888       d88P  Y88b
    // 88888b.d88888 888       Y88b.      Y88b.          d88P888 888    888 888       Y88b.
    // 888Y88888P888 8888888    "Y888b.    "Y888b.      d88P 888 888        8888888    "Y888b.
    // 888 Y888P 888 888           "Y88b.     "Y88b.   d88P  888 888  88888 888           "Y88b.
    // 888  Y8P  888 888             "888       "888  d88P   888 888    888 888             "888
    // 888   "   888 888       Y88b  d88P Y88b  d88P d8888888888 Y88b  d88P 888       Y88b  d88P
    // 888       888 8888888888 "Y8888P"   "Y8888P" d88P     888  "Y8888P88 8888888888 "Y8888P"

    static bool on_messages_acquire(void* user_ctx, u32 data_type, u32 data_size, nnet::buffer_t& buffer)
    {
        app_data_t* app_data = (app_data_t*)user_ctx;

        switch (data_type)
        {
            case nnet::MSG_TYPE_DOWNLOAD_COMPLETE:
                app_data->m_download_finished = true;
                buffer.m_buffer               = nullptr;
                buffer.m_length               = 0;
                return true;
        }
        return false;
    }

    static void on_messages_complete(void* user_ctx, u32 data_type, nnet::buffer_t buffer)
    {
        // todo
    }

    static void on_messages_abort(void* on_abort_context, u32 data_type, nnet::buffer_t buffer)
    {
        // todo
    }

    // -------------------------------------------------------------------------------------------
    enum asset_server_state_t
    {
        ASSET_SERVER_STATE_SETUP = 0,
        ASSET_SERVER_STATE_CONNECTING,
        ASSET_SERVER_STATE_CONNECTED,
        ASSET_SERVER_STATE_DOWNLOADING,
        ASSET_SERVER_STATE_COMPLETE,
        ASSET_SERVER_STATE_ERROR,
    };

    struct data_type_version_t
    {
        u32 type;
        u32 version;
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
            nnet::tcp_recv_plugin_t* messages_plugin = nnet::new_messages_plugin(on_messages_acquire, on_messages_complete, on_messages_abort, &app_data);

            // Register the necessary plugins with the TCP client
            nnet::register_plugin(app_data->m_tcp_client, 0, download_plugin);
            nnet::register_plugin(app_data->m_tcp_client, 1, messages_plugin);

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
            // Send download_request_msg_t to the server to initiate the download process
            //
            u8 const*    mac              = nnet::get_mac_address(app_data->m_wifi_manager);
            nnet::msg_t& download_request = app_data->m_msg;
            nnet::msg_init(download_request, nnet::MSG_TYPE_DOWNLOAD_REQUEST, mac);

            data_type_version_t assets[DATA_TYPE_COUNT];
            assets[DATA_TYPE_SPRITE_PACK]   = {DATA_TYPE_SPRITE_PACK, 0};
            assets[DATA_TYPE_FONT_PACK]     = {DATA_TYPE_FONT_PACK, 0};
            assets[DATA_TYPE_PALETTE_PACK]  = {DATA_TYPE_PALETTE_PACK, 0};
            assets[DATA_TYPE_SCRIPT_BINARY] = {DATA_TYPE_SCRIPT_BINARY, 0};
            assets[DATA_TYPE_HOUSE_META]    = {DATA_TYPE_HOUSE_META, 0};

            if (app_data->m_sprite_pack_size > 0 && app_data->m_sprite_pack != nullptr)
            {
                assets[DATA_TYPE_SPRITE_PACK] = {DATA_TYPE_SPRITE_PACK, app_data->m_sprite_pack->m_version};
            }
            if (app_data->m_font_pack_size > 0 && app_data->m_font_pack != nullptr)
            {
                assets[DATA_TYPE_FONT_PACK] = {DATA_TYPE_FONT_PACK, app_data->m_font_pack->m_version};
            }
            if (app_data->m_palette_pack_size > 0 && app_data->m_palette_pack != nullptr)
            {
                assets[DATA_TYPE_PALETTE_PACK] = {DATA_TYPE_PALETTE_PACK, app_data->m_palette_pack->m_version};
            }
            if (app_data->m_script_binary_size > 0 && app_data->m_script_binary != nullptr)
            {
                assets[DATA_TYPE_SCRIPT_BINARY] = {DATA_TYPE_SCRIPT_BINARY, app_data->m_script_binary->m_version};
            }
            if (app_data->m_house_meta_size > 0 && app_data->m_house_meta != nullptr)
            {
                assets[DATA_TYPE_HOUSE_META] = {DATA_TYPE_HOUSE_META, app_data->m_house_meta->m_version};
            }

            for (i32 i = 0; i < DATA_TYPE_COUNT; ++i)
            {
                nnet::msg_write(download_request, assets[i].type);
                nnet::msg_write(download_request, assets[i].version);
            }

            nnet::msg_final(download_request);
            if (nnet::send(app_data->m_tcp_client, download_request.Data, download_request.Size))
            {
                state_data->m_state_data = ASSET_SERVER_STATE_DOWNLOADING;
            }
        }
        else if (state_data->m_state_data == ASSET_SERVER_STATE_DOWNLOADING)
        {
            // Wait until we have received all the necessary data from the asset server:
            // - sprite pack
            // - font pack
            // - palette pack
            // - script binary
            // - house meta data

            // Once we have received a message from the server with type MSG_TYPE_DOWNLOAD_COMPLETE, then we know
            // that the download has finished.
            if (app_data->m_download_finished)
            {
                // Fully disconnect the TCP client from the asset server.
                nnet::disconnect(app_data->m_tcp_client);

                // TODO
                // Possibly verify the integrity of the downloaded assets here (e.g., checksum, signature) before proceeding.

                // Also free the plugins and their associated data, as they are no longer needed.
                nnet::tcp_recv_plugin_t* download_plugin = nnet::get_plugin(app_data->m_tcp_client, 0);
                nnet::tcp_recv_plugin_t* messages_plugin = nnet::get_plugin(app_data->m_tcp_client, 1);
                nnet::unregister_plugin(app_data->m_tcp_client, 0, download_plugin);
                nnet::unregister_plugin(app_data->m_tcp_client, 1, messages_plugin);

                // Ready to move on to the next state in the FSM.
                to_state_next(state_data);
            }
        }
    }

}  // namespace ncore