#include "rcore/c_app.h"
#include "rcore/c_gpio.h"
#include "rcore/c_timer.h"
#include "rcore/c_log.h"
#include "rcore/c_packet.h"
#include "rcore/c_str.h"
#include "rcore/c_system.h"
#include "rcore/c_wire.h"

#include "ccore/c_memory.h"
#include "ccore/c_random.h"

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"

#include "lib_wcs/c_lcd.h"
#include "lib_wcs/c_touch.h"
#include "lib_wcs/c_sdcard.h"

#include "cfenc/c_codec.h"
#include "cfenc/c_decoder.h"

#include "mui/c_display.h"
#include "mui/c_display_format.h"
#include "mui/c_mui_client.h"
#include "mui/c_client_msg.h"

namespace ncore
{
    namespace nmui
    {
        const nmui::known_lcd_boards_t gLcdBoard = nmui::LCD_BOARD_WCS_320X480_3_5_INCH;

        struct mui_client_t
        {
            nmui::display_t m_display;
            mui_state_t     m_state;

            u8                 m_mac_address[6];  // MAC address of the client device
            ntcp::tcp_client_t m_tcp_client;
            ntcp::config_t     m_tcp_config;

            u32                  m_tiles_data[2 * 1024];  // max 1024 x 1024 screen resolution with 16x16 tiles
            nfenc::dirty_tiles_t m_dirty_tiles;

            ntimer::periodic_task_t m_frame_request_task;

            nfenc::frame_begin_t m_header;
            u16                  m_spans_per_line;
            u32                  m_msg_buffer[2048];
        };

        static mui_client_t gMuiClient;

        static void clear_screen(nmui::display_t& display)
        {
            const u32 screen_pixel_count = display.m_width * display.m_height;
            for (u32 i = 0; i < screen_pixel_count; i++)
                display.m_fb[i] = 0x0000;
        }

        static void clear_tile_data(mui_client_t* mui_client)
        {
            for (u32 i = 0; i < DARRAYSIZE(mui_client->m_tiles_data); i++)
                mui_client->m_tiles_data[i] = 0;
        }

        // -------------------------------------------------------------------------------------------
        // TCP receiving, framing state (zero-copy)
        // -------------------------------------------------------------------------------------------
        static ntcp::tcp_buffer_t tcp_recv_acquire(void* ctx, void* _hdr)
        {
            const u16* hdr      = (const u16*)_hdr;
            const u16  msg_type = hdr[0];
            const u16  msg_len  = hdr[1];

            ntcp::tcp_buffer_t buffer;
            if (msg_type == nfenc::MSG_ID_FRAME_BEGIN)
            {
                // frame begin message
                buffer.m_buffer = (u8*)&gMuiClient.m_header;
                buffer.m_length = msg_len - 4;  // subtract the size of the header
                return buffer;
            }
            else if (msg_type == nfenc::MSG_ID_FRAME_LINE)
            {
                // frame line message, we use the ring buffer to receive the line data
                buffer.m_buffer = (u8*)gMuiClient.m_msg_buffer;
                buffer.m_length = msg_len - 4;
                return buffer;
            }
            else if (msg_type == nfenc::MSG_ID_FRAME_END)
            {
                // frame end message, we don't have any data to receive, just return a dummy buffer
                buffer.m_buffer = (u8*)gMuiClient.m_msg_buffer;
                buffer.m_length = msg_len - 4;
                return buffer;
            }

            buffer.m_buffer = (u8*)gMuiClient.m_msg_buffer;
            buffer.m_length = 0;
            return buffer;
        }

        static void tcp_recv_commit(void* ctx, void* _hdr, ntcp::tcp_buffer_t buffer)
        {
            // process the received message here
            const u16* hdr      = (const u16*)_hdr;
            const u16  msg_type = hdr[0];

            if (msg_type == nfenc::MSG_ID_FRAME_BEGIN)
            {
                gMuiClient.m_state = MUI_STATE_RECEIVING_FRAME;

                // frame begin message, we can initialize the frame data here
                gMuiClient.m_spans_per_line = (gMuiClient.m_header.m_img_width + gMuiClient.m_header.m_tile_width - 1) / gMuiClient.m_header.m_tile_width;

                // initialize
                const u32 num_tiles_per_line                 = (gMuiClient.m_header.m_img_width + gMuiClient.m_header.m_tile_width - 1) / gMuiClient.m_header.m_tile_width;
                gMuiClient.m_dirty_tiles.m_tile_width_shift  = gMuiClient.m_header.m_tile_width;
                gMuiClient.m_dirty_tiles.m_tile_height_shift = gMuiClient.m_header.m_tile_height;
                gMuiClient.m_dirty_tiles.m_tiles_stride      = (num_tiles_per_line + 31) / 32;  // number of u32 words needed to store one row of tiles
                gMuiClient.m_dirty_tiles.m_tiles_cols        = (gMuiClient.m_header.m_img_width + gMuiClient.m_header.m_tile_width - 1) / gMuiClient.m_header.m_tile_width;
                gMuiClient.m_dirty_tiles.m_tiles_rows        = (gMuiClient.m_header.m_img_height + gMuiClient.m_header.m_tile_height - 1) / gMuiClient.m_header.m_tile_height;
                gMuiClient.m_dirty_tiles.m_tiles_data        = (u32*)gMuiClient.m_tiles_data;
                nfenc::clear(gMuiClient.m_dirty_tiles);
            }

            else if (msg_type == nfenc::MSG_ID_FRAME_LINE)
            {
                // frame line message, we can process the line data here
                nfenc::decode_line(gMuiClient.m_header, gMuiClient.m_header.m_tile_width, gMuiClient.m_spans_per_line, (const nfenc::frame_line_t*)buffer.m_buffer, gMuiClient.m_display.m_fb);
                nfenc::update_tiles(gMuiClient.m_header, (const nfenc::frame_line_t*)buffer.m_buffer, gMuiClient.m_dirty_tiles);
            }
            else if (msg_type == nfenc::MSG_ID_FRAME_END)
            {
                // mark a new frame as ready to be rendered
                gMuiClient.m_state = MUI_STATE_RENDERING_FRAME;
            }
        }

        static void tcp_recv_abort(void* ctx)
        {
            // to be implemented
        }

        static void user_tcp_connected(void* ctx)
        {
            mui_client_t* mui_client = (mui_client_t*)ctx;

            client_info_t client_info;
            client_info.m_message_type   = (u16)'C' << 8 | (u16)'I';  // MessageTypeClientInfo
            client_info.m_message_len    = sizeof(client_info_t);
            client_info.m_display_format = ncore::nmui::get_display_format(gLcdBoard);
            client_info.m_screen_width   = ncore::nmui::get_display_width(gLcdBoard);
            client_info.m_screen_height  = ncore::nmui::get_display_height(gLcdBoard);
            g_memcpy(client_info.m_mac_address, mui_client->m_mac_address, 6);

            ntcp::tcp_client_t* tcp_client = &mui_client->m_tcp_client;
            ntcp::send(*tcp_client, &client_info, sizeof(client_info));

            // when just connected, clear the screen to black
            clear_screen(gMuiClient.m_display);
        }

        // -------------------------------------------------------------------------------------------
        // -------------------------------------------------------------------------------------------
        static void s_request_frame(void* user)
        {
            mui_client_t* mui_client = (mui_client_t*)user;

            if (mui_client->m_state == MUI_STATE_ACTIVE)
            {
                // send a message to server to request a new frame
                u16                 msg_type   = (u16)'R' << 8 | (u16)'F';  // MessageTypeRequestFrame
                ntcp::tcp_client_t* tcp_client = &mui_client->m_tcp_client;
                ntcp::send(*tcp_client, &msg_type, sizeof(msg_type));
            }
        }

        bool init_mui_client(nwifi::wifi_manager_t* wifi_mgr)
        {
            mui_client_t* mui = &gMuiClient;

            mui->m_state = MUI_STATE_ACTIVE;
            mui->m_frame_request_task;
            ntimer::init_periodic_task(&mui->m_frame_request_task, 5057, mui, s_request_frame);  // request a new frame every 5 seconds

            nmui::init_display(mui->m_display, ncore::nmui::get_display_width(gLcdBoard), ncore::nmui::get_display_height(gLcdBoard));
            clear_screen(mui->m_display);
            clear_tile_data(mui);

            g_memclr(&mui->m_header, sizeof(mui->m_header));
            mui->m_spans_per_line = mui->m_display.m_width / 16;  // assuming tile width is 16, this will be updated when we receive the frame begin message
            g_memclr(mui->m_msg_buffer, sizeof(mui->m_msg_buffer));

            ntcp::tcp_client_t* tcp_client                 = &gMuiClient.m_tcp_client;
            tcp_client->m_user                             = &gMuiClient;
            tcp_client->m_user_callbacks.m_on_connected    = user_tcp_connected;
            tcp_client->m_user_callbacks.m_on_disconnected = nullptr;

            ntcp::config_t* tcp_config       = &gMuiClient.m_tcp_config;
            void*           tcp_socket       = ntcp::setup_default(tcp_config);
            tcp_config->m_recv_ops.m_acquire = tcp_recv_acquire;
            tcp_config->m_recv_ops.m_commit  = tcp_recv_commit;
            tcp_config->m_recv_ops.m_abort   = tcp_recv_abort;
            ntcp::setup(*tcp_client, tcp_config, tcp_socket, REMOTE_UI_SERVER_IP(), REMOTE_UI_SERVER_TCPPORT(), nullptr, 4);

            // connect to the server, the client_info will be sent whenever we are connected (also on every reconnect)
            ntcp::connect(*tcp_client);

            return true;
        }

        mui_state_t tick_mui_client(nwifi::wifi_manager_t* wifi_mgr, u64 now_ms)
        {
            // state machine:
            // - if connected and a new frame is ready, render it and clear dirty tiles
            // - when in the state of receiving a frame and rendering it, we will not poll any
            //   of the sensors (temperature, humidity, pressure, light, presence) to avoid any
            //   potential interference with the TCP receiving and rendering performance, we will
            //   only poll the sensors when we are in the idle state (not receiving or rendering a frame)
            // - according to frame-rate, request a new frame by sending a message to the server
            //   - only if display is ON, this is determined by the system having a mmWave sensor
            //     detecting presence, if no presence then display is turned OFF.
            // - on any input event (touch, button), send an input message to the server, this also
            //   puts the system in the ON state (display ON, request frames from server, poll sensors, etc.)

            ntcp::tick_tcp_client(wifi_mgr, gMuiClient.m_tcp_client);

            ntimer::tick_periodic_task(&gMuiClient.m_frame_request_task, now_ms);

            if (gMuiClient.m_state == MUI_STATE_RENDERING_FRAME)
            {
                // render the frame
                nmui::render_tiles(gMuiClient.m_display, gMuiClient.m_dirty_tiles);

                // after rendering, clear the dirty tiles
                nfenc::clear(gMuiClient.m_dirty_tiles);

                // mark the frame as rendered
                gMuiClient.m_state = MUI_STATE_ACTIVE;

                // reset the frame request timer so that we don't request a new frame immediately after rendering
                mark_periodic_task(&gMuiClient.m_frame_request_task, now_ms);
            }

            return gMuiClient.m_state;
        }

        void send_input_event(u8 const* event_data, u16 event_len)
        {
            // send the input event to the server
            ntcp::send(gMuiClient.m_tcp_client, event_data, event_len);
        }

    }  // namespace nmui
}  // namespace ncore