#ifndef __RMUI_CLIENT_INFO_H__
#define __RMUI_CLIENT_INFO_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"
#include "mui/c_input_msg.h"

namespace ncore
{
    namespace nmui
    {
        bool init_mui_client(nwifi::wifi_manager_t* wifi_mgr);

        enum mui_state_t
        {
            // DISPLAY OFF; not requesting frames from server, we are polling sensors and sending sensor data
            // to server, but we are not requesting frames from server, this is determined by the presence
            // detection (if no presence then display is turned OFF)
            MUI_STATE_DISPLAY_OFF,

            // ACTIVE; display is ON, polling sensors, requesting frames from server, etc.
            MUI_STATE_ACTIVE,

            // RECEIVING and RENDERING; when we are in the state of receiving a frame and rendering it, we will not poll any
            // of the sensors (temperature, humidity, pressure, light, presence) to avoid any potential interference with the
            // TCP receiving and rendering performance, we will only poll the sensors when we are in the idle state
            MUI_STATE_RECEIVING_FRAME,
            MUI_STATE_RENDERING_FRAME,
        };

        mui_state_t tick_mui_client(nwifi::wifi_manager_t* wifi_mgr, u64 now_ms);
        void        send_input_event(u8 const* event_data, u16 event_len);

    }  // namespace nmui
}  // namespace ncore

#endif  // __RMUI_CLIENT_INFO_H__