#ifndef __C_APP_DATA_H__
#define __C_APP_DATA_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "ccore/c_random.h"
#include "cgx2/c_types.h"

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"

#include "lib_guition/c_lcd.h"
#include "lib_guition/c_sdcard.h"

#include "lib_touch/c_touch_gt911.h"
#include "lib_touch/c_touch_gesture.h"

namespace ncore
{
    struct app_data_t;

    namespace ngx2
    {
        struct sprite_pack_t;
        struct font_pack_t;
        struct palette_pack_t;
    }  // namespace ngx2

    ngx2::sprite_t*  get_sprite(app_data_t* data, u32 index);
    ngx2::palette_t* get_palette(app_data_t* data, u32 index);
    ngx2::font_t*    get_font(app_data_t* data, u32 index);
    const void*      get_script(app_data_t* data, u32* script_binary_size);

    enum fsm_state_enum_t
    {
        FSM_STATE_NONE = 0,
        FSM_STATE_CONNECT_TO_WIFI,
        FSM_STATE_DOWNLOAD_FROM_ASSET_SERVER,
        FSM_STATE_CONNECT_TO_SENSOR_SERVER,
        FSM_STATE_INITIALIZE_SENSORS,
        FSM_STATE_INITIALIZE_DISPLAY,
        FSM_STATE_SHOW_SPLASH_SCREEN,
        FSM_STATE_INITIALIZE_TOUCH,
        FSM_STATE_INITIALIZE_SDCARD,
        FSM_STATE_INITIALIZE_SCRIPT_VM,
        FSM_STATE_ACTIVE_DISPLAY_ON,
        FSM_STATE_ACTIVE_DISPLAY_OFF,
        FSM_STATE_ERROR,
        FSM_STATE_COUNT,
    };

    struct fsm_state_data_t
    {
        i32              m_state_data;      // State-specific data that can be used to store information relevant to the current state
        fsm_state_enum_t m_current_state;   // The current state of the FSM
        fsm_state_enum_t m_previous_state;  // The previous state before the current state
        fsm_state_enum_t m_next_state;      // The next state to transition to after leaving the current state
    };

    typedef void (*state_fn_t)(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);

    void handle_state_data(fsm_state_data_t& state_data, app_data_t& app_data);

    void state_connect_to_wifi(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);
    void state_download_from_asset_server(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);

    void state_connect_to_sensor_server(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);
    void state_initialize_sensors(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);

    void state_initialize_display(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);
    void state_show_splash_screen(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);
    void state_initialize_touch(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);
    void state_initialize_sdcard(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);

    void state_initialize_script_vm(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);

    void state_active_display_on(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);
    void state_active_display_off(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);

    void state_error(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms);

    struct app_data_t
    {
        xor_random_t            m_random;         // XORShift random number generator for any randomization needs
        ntouch::touch_t         m_touch;          // Touch
        ntouch::touch_gesture_t m_touch_gesture;  // Touch gesture state

        nnet::wifi_manager_t m_wifi_manager;  // WiFi manager for network connectivity
        nnet::wifi_config_t  m_wifi_config;   // WiFi configuration

        nnet::config_t     m_tcpclient_config;         // TCP client configuration (shared)
        nnet::tcp_client_t m_tcpclient_asset_server;   // TCP client for server communication
        nnet::tcp_client_t m_tcpclient_sensor_server;  // TCP client for server communication

        ngx2::sprite_pack_t*  m_sprites;             // pointer to the array of assets in PSRAM
        ngx2::font_pack_t*    m_fonts;               // pointer to the array of fonts in PSRAM
        ngx2::palette_pack_t* m_palettes;            // pointer to the array of palettes in PSRAM
        void*                 m_script_binary;       // pointer to the script binary in PSRAM
        u32                   m_script_binary_size;  // size of the script binary in bytes

        fsm_state_data_t m_state_data;                 // Current state call type (enter, update, leave)
        state_fn_t       m_state_fn[FSM_STATE_COUNT];  // Array of state functions for each application state
    };

    void init_app_data(app_data_t& app_data);

    void update_touch(app_data_t& data, u64 now_ms);

}  // namespace ncore

#endif  // __C_APP_DATA_H__
