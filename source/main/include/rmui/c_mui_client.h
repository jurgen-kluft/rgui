#ifndef __RMUI_CLIENT_INFO_H__
#define __RMUI_CLIENT_INFO_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"

namespace ncore
{
    namespace nmui
    {
        struct client_info_t
        {
            u16  m_message_type;        // should be MessageTypeClientInfo
            u16  m_message_len;         // 16 (fixed size of the ClientInfo struct)
            u16  m_display_format;      // see display_format_t
            u16  m_screen_width;        // in pixels
            u16  m_screen_height;       // in pixels
            u8   m_mac_address[6];      // MAC address of the client device
        };

        bool init_mui_client(ntcp::tcp_client_t* tcp_client, ntcp::config_t* tcp_config, nwifi::wifi_manager_t* wifi_mgr);

    }  // namespace nmui
}  // namespace ncore

#endif  // __RMUI_CLIENT_INFO_H__