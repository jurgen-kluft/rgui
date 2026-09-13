#ifndef __C_APP_DATA_H__
#define __C_APP_DATA_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "ccore/c_random.h"

#include "cgx2/c_types.h"
#include "rhome/c_home.h"

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"

#include "lib_guition/c_lcd.h"
#include "lib_guition/c_sdcard.h"

#include "lib_touch/c_touch_gt911.h"
#include "lib_touch/c_touch_gesture.h"

namespace ncore
{
    struct app_data_t;
    struct linked_program_t;

    enum fsm_state_enum_t
    {
        FSM_STATE_NONE = 0,              // No state, initial state before any initialization
        FSM_STATE_HANDLE_LITTLE_FS,      // Initialize the LittleFS
        FSM_STATE_HANDLE_SD_CARD,        // Initialize the SD card
        FSM_STATE_VERIFY_ASSETS,         // Do we have all the necessary assets
        FSM_STATE_CONNECT_TO_WIFI,       // First, connect to WiFi to enable network communication
        FSM_STATE_DOWNLOAD_ASSETS,       // Next, download assets from the asset server
        FSM_STATE_VERIFY_ASSETS,         // Verify that all necessary assets are present before downloading
        FSM_STATE_HANDLE_SENSOR_SERVER,  // Next, connect to the sensor server
        FSM_STATE_INITIALIZE_SENSORS,    // Next, initialize the sensors
        FSM_STATE_INITIALIZE_DISPLAY,    // Next, initialize the display
        FSM_STATE_SHOW_SPLASH_SCREEN,    // Next, show the splash screen
        FSM_STATE_INITIALIZE_TOUCH,      // Next, initialize the touch interface
        FSM_STATE_INITIALIZE_SCRIPT_VM,  // Next, initialize the script VM
        FSM_STATE_ACTIVE,                // This is the active state
        FSM_STATE_ERROR,                 // Error state, entered when an unrecoverable error occurs
        FSM_STATE_COUNT,
    };

    struct fsm_state_data_t
    {
        i32              m_state_data;      // State-specific data that can be used to store information relevant to the current state
        fsm_state_enum_t m_current_state;   // The current state of the FSM
        fsm_state_enum_t m_previous_state;  // The previous state before the current state
        fsm_state_enum_t m_next_state;      // The next state to transition to after leaving the current state
    };

    inline void to_state_error(fsm_state_data_t* state_data) { state_data->m_next_state = FSM_STATE_ERROR; }
    inline void to_state_next(fsm_state_data_t* state_data) { state_data->m_next_state = (fsm_state_enum_t)(state_data->m_current_state + 1); }

    typedef void (*state_fn_t)(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);

    void handle_state_data(fsm_state_data_t* state_data, app_data_t* app_data);

    void state_handle_little_fs(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);
    void state_handle_sd_card(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);

    void state_connect_to_wifi(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);
    void state_download_assets(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);
    void state_verify_assets(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);

    void state_handle_sensor_server(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);
    void state_initialize_sensors(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);

    void state_initialize_display(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);
    void state_show_splash_screen(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);
    void state_initialize_touch(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);

    void state_initialize_script_vm(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);

    void state_active(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);

    void state_error(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms);

    enum edata_source_t
    {
        EDATA_SOURCE_NONE = 0,
        EDATA_SOURCE_LITTLE_FS,
        EDATA_SOURCE_SD_CARD,
        EDATA_SOURCE_ASSET_SERVER,
    };

    struct app_data_t
    {
        house_t* m_house;

        bool m_sd_card_available;
        bool m_little_fs_available;

        xor_random_t            m_random;         // XORShift random number generator for any randomization needs
        ntouch::touch_t         m_touch;          // Touch
        ntouch::touch_gesture_t m_touch_gesture;  // Touch gesture state

        nnet::wifi_manager_t m_wifi_manager;  // WiFi manager for network connectivity
        nnet::wifi_config_t  m_wifi_config;   // WiFi configuration

        nnet::config_t     m_tcpclient_config;         // TCP client configuration (shared)
        nnet::tcp_client_t m_tcpclient_asset_server;   // TCP client for server communication
        nnet::tcp_client_t m_tcpclient_sensor_server;  // TCP client for server communication

        edata_source_t        m_sprite_pack_source;      // Source of the sprite pack data (e.g., LITTLE_FS, SD_CARD, ASSET_SERVER)
        u32                   m_sprite_pack_size;        // size of the sprite pack in bytes
        u32                   m_sprite_pack_capacity;    // capacity of allocated sprite pack memory
        ngx2::sprite_pack_t*  m_sprite_pack;             // pointer to the array of assets in PSRAM
        edata_source_t        m_font_pack_source;        // Source of the font pack data (e.g., LITTLE_FS, SD_CARD, ASSET_SERVER)
        u32                   m_font_pack_size;          // size of the font pack in bytes
        u32                   m_font_pack_capacity;      // capacity of allocated font pack memory
        ngx2::font_pack_t*    m_font_pack;               // pointer to the array of fonts in PSRAM
        edata_source_t        m_palette_pack_source;     // Source of the palette pack data (e.g., LITTLE_FS, SD_CARD, ASSET_SERVER)
        u32                   m_palette_pack_size;       // size of the palette pack in bytes
        u32                   m_palette_pack_capacity;   // capacity of allocated palette pack memory
        ngx2::palette_pack_t* m_palette_pack;            // pointer to the array of palettes in PSRAM
        edata_source_t        m_script_binary_source;    // Source of the script binary data (e.g., LITTLE_FS, SD_CARD, ASSET_SERVER)
        u32                   m_script_binary_size;      // size of the script binary in bytes
        u32                   m_script_binary_capacity;  // capacity of allocated script binary memory
        linked_program_t*     m_script_binary;           // pointer to the script binary in PSRAM

        fsm_state_data_t m_state_data;                 // Current state call type (enter, update, leave)
        state_fn_t       m_state_fn[FSM_STATE_COUNT];  // Array of state functions for each application state
    };

    bool get_script(app_data_t* data, const void*& script_binary, u32& script_binary_size, u32& script_memory_capacity);

    void init_app_data(app_data_t* app_data);

    void update_touch(app_data_t* data, u64 now_ms);
    void update_script(app_data_t* data, u64 now_ms);

}  // namespace ncore

#endif  // __C_APP_DATA_H__
