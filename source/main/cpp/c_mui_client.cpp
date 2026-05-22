#include "rcore/c_app.h"
#include "rcore/c_gpio.h"
#include "rcore/c_timer.h"
#include "rcore/c_log.h"
#include "rcore/c_packet.h"
#include "rcore/c_str.h"
#include "rcore/c_system.h"
#include "rcore/c_task.h"
#include "rcore/c_wire.h"

#include "ccore/c_memory.h"
#include "ccore/c_random.h"

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"
#include "rwifi/c_network_mgr.h"

#include "lib_wcs/c_lcd.h"
#include "lib_wcs/c_touch.h"
#include "lib_wcs/c_sdcard.h"

#include "cfenc/c_codec.h"
#include "cfenc/c_decoder.h"

#include "rmui/c_display.h"
#include "rmui/c_mui_client.h"

namespace ncore
{
    namespace nmui
    {
        struct mui_client_t
        {
            nwifi::wifi_manager_t* m_wifi_mgr;
            ntcp::tcp_client_t*    m_tcp_client;

            nfenc::frame_begin_t m_header;
            u16*                 m_psram_frame_buffer;  // pointer to the framebuffer in PSRAM where the decoded frame will be stored
            u16                  m_spans_per_line;
            u32                  m_msg_buffer[2048];
        };

        static mui_client_t gMuiClient;

        // -------------------------------------------------------------------------------------------
        // TCP receiving, framing state (zero-copy)
        // -------------------------------------------------------------------------------------------
        static ntcp::tcp_buffer_t tcp_recv_acquire(void* ctx, void* _hdr)
        {
            const u16* hdr      = (const u16*)_hdr;
            const u16  msg_type = hdr[0];
            const u16  msg_len  = hdr[1];

            if (msg_type == nfenc::MSG_ID_FRAME_BEGIN)
            {
                // frame begin message
                return ntcp::tcp_buffer_t((u8*)&gMuiClient.m_header, msg_len - 4);
            }
            else if (msg_type == nfenc::MSG_ID_FRAME_LINE)
            {
                // frame line message, we use the ring buffer to receive the line data
                u8* line_data = (u8*)gMuiClient.m_msg_buffer;
                return ntcp::tcp_buffer_t(line_data, msg_len - 4);
            }
        }

        static void tcp_recv_commit(void* ctx, void* _hdr, ntcp::tcp_buffer_t buffer)
        {
            // process the received message here
            const u16* hdr      = (const u16*)_hdr;
            const u16  msg_type = hdr[0];

            if (msg_type == nfenc::MSG_ID_FRAME_BEGIN)
            {
                // frame begin message, we can initialize the frame data here
                gMuiClient.m_spans_per_line = (gMuiClient.m_header.m_img_width + gMuiClient.m_header.m_tile_width - 1) / gMuiClient.m_header.m_tile_width;
            }
            else if (msg_type == nfenc::MSG_ID_FRAME_LINE)
            {
                // frame line message, we can process the line data here
                nfenc::decode_line(gMuiClient.m_header, gMuiClient.m_header.m_tile_width, gMuiClient.m_spans_per_line, (const nfenc::frame_line_t*)buffer.m_buffer, gMuiClient.m_psram_frame_buffer);
            }
        }

        static void tcp_recv_abort(void* ctx)
        {
            // to be implemented
        }

        static void user_tcp_connected(void* ctx)
        {
            mui_client_t* mui_client = (mui_client_t*)ctx;

            // send nfenc::client_info_t
            client_info_t client_info;
            client_info.m_message_type   = 'CI';  // MessageTypeClientInfo
            client_info.m_message_len    = sizeof(client_info_t);
            client_info.m_display_format = 0;    // TODO set the actual display format
            client_info.m_screen_width   = 320;  // TODO set the actual screen width
            client_info.m_screen_height  = 480;  // TODO set the actual MAC address of the client device

            nwifi::wifi_manager_t* wifi_mgr = mui_client->m_wifi_mgr;
            g_memcpy(client_info.m_mac_address, wifi_mgr->m_mac_address, 6);

            ntcp::tcp_client_t* tcp_client = mui_client->m_tcp_client;
            ntcp::send(*tcp_client, &client_info, sizeof(client_info));
        }

        // -------------------------------------------------------------------------------------------
        // -------------------------------------------------------------------------------------------

        bool init_mui_client(ntcp::tcp_client_t* tcp_client, ntcp::config_t* tcp_config, nwifi::wifi_manager_t* wifi_mgr)
        {
            gMuiClient.m_wifi_mgr   = wifi_mgr;
            gMuiClient.m_tcp_client = tcp_client;

            // allocate a framebuffer for 320x480 display
            gMuiClient.m_psram_frame_buffer = (u16*)nsystem::alloc_psram(320 * 480 * sizeof(u16));
            if (!gMuiClient.m_psram_frame_buffer)
                return false;

            tcp_config->m_recv_ops.m_acquire = tcp_recv_acquire;
            tcp_config->m_recv_ops.m_commit  = tcp_recv_commit;
            tcp_config->m_recv_ops.m_abort   = tcp_recv_abort;

            tcp_client->m_user                             = &gMuiClient;
            tcp_client->m_user_callbacks.m_on_connected    = user_tcp_connected;
            tcp_client->m_user_callbacks.m_on_disconnected = nullptr;

            void* socket = ntcp::setup_default(tcp_config);
            ntcp::setup(*tcp_client, tcp_config, socket, SERVER_IP(), SERVER_TCPPORT(), nullptr, 4);
            ntcp::activate(*tcp_client);
            return true;
        }
    }  // namespace nmui
}  // namespace ncore