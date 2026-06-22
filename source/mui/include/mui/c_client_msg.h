#ifndef __RMUI_CLIENT_MESSAGES_H__
#define __RMUI_CLIENT_MESSAGES_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nmui
    {
        enum
        {
            MSG_ID_CLIENT_INFO   = 0x4349,  // 'CI' in ASCII, client_info_t
            MSG_ID_FRAME_REQUEST = 0x4652,  // 'FR' in ASCII, frame_request_t
        };

        struct client_info_t
        {
            u16 m_message_type;    // should be MessageTypeClientInfo
            u16 m_message_len;     // 16 (fixed size of the ClientInfo struct)
            u16 m_display_format;  // see display_format_t
            u16 m_screen_width;    // in pixels
            u16 m_screen_height;   // in pixels
            u8  m_mac_address[6];  // MAC address of the client device
        };

        struct frame_request_t
        {
            u16 m_message_type;  // should be MessageTypeFrameRequest
            u16 m_message_len;   // 8 (fixed size of the FrameRequest struct)
            u16 m_ms_per_frame;  // requested milliseconds per frame (adaptive frame-rate control by the client)
            u16 m_flags;         // bit 0 if non-zero, request the server to send the next frame as an I-frame (full frame), otherwise it can be a P-frame (delta frame);
        };

    }  // namespace nmui
}  // namespace ncore

#endif  // __RMUI_CLIENT_MESSAGES_H__