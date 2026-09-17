#include "rcore/c_app.h"
#include "rcore/c_gpio.h"
#include "rcore/c_timer.h"
#include "rcore/c_log.h"
#include "rcore/c_str.h"
#include "rcore/c_system.h"
#include "rcore/c_wire.h"

#include "ccore/c_memory.h"
#include "ccore/c_random.h"

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"

#include "lib_guition/c_lcd.h"
#include "lib_guition/c_sdcard.h"

#include "lib_touch/c_touch_gt911.h"
#include "lib_touch/c_touch_gesture.h"

#include "gui/c_display.h"
#include "gui/c_draw.h"

#include "main/c_app_data.h"

namespace ncore
{
    void init_app_data(app_data_t* app_data)
    {
        g_memclr(app_data, sizeof(app_data_t));

        app_data->m_random.reset(0x1234567890abcdef);  // Initialize the random number generator with a fixed seed for reproducibility

        ntouch::gesture_config_t gesture_config;
        ntouch::init_touch_gesture(app_data->m_touch_gesture, gesture_config);

        nnet::init_wifi_config(app_data->m_wifi_config, WIFI_SSID(), WIFI_PASSWORD(), 1000, 16000, 2.0f, 0.1f);
        nnet::setup_default(&app_data->m_tcp_client_config);

        // The sizes are kept zero, to indicate that no data is currently stored in the allocated memory blocks.
        app_data->m_sprite_pack_capacity   = 5 * cMB;   // PSRAM capacity of allocated sprite pack memory
        app_data->m_font_pack_capacity     = 64 * cKB;  // PSRAM capacity of allocated font pack memory
        app_data->m_palette_pack_capacity  = 8 * cKB;   // SRAM capacity of allocated palette pack memory
        app_data->m_script_binary_capacity = 32 * cKB;  // SRAM capacity of allocated script binary memory
        app_data->m_house_meta_capacity    = 4 * cKB;   // SRAM capacity of allocated house meta memory
        app_data->m_house_global_capacity  = 2 * cKB;   // SRAM capacity of allocated house global memory
        app_data->m_house_sensors_capacity = 2 * cKB;   // SRAM capacity of allocated house sensors memory
        app_data->m_sprite_pack            = (ngx2::sprite_pack_t*)nsystem::alloc_psram_aligned(app_data->m_sprite_pack_capacity, 32);
        app_data->m_font_pack              = (ngx2::font_pack_t*)nsystem::alloc_psram_aligned(app_data->m_font_pack_capacity, 32);
        app_data->m_palette_pack           = (ngx2::palette_pack_t*)nsystem::alloc_psram_aligned(app_data->m_palette_pack_capacity, 32);
        app_data->m_script_binary          = (linked_program_t*)nsystem::malloc(app_data->m_script_binary_capacity);
        app_data->m_house_meta             = (house_t*)nsystem::malloc(app_data->m_house_meta_capacity);
        app_data->m_house_global           = (house_global_t*)nsystem::malloc(app_data->m_house_global_capacity);
        app_data->m_house_sensors          = (byte*)nsystem::malloc(app_data->m_house_sensors_capacity);

        fsm_state_data_t* state_data = &app_data->m_state_data;
        state_data->m_state_data     = 0;
        state_data->m_current_state  = FSM_STATE_CONNECT_TO_WIFI;
        state_data->m_previous_state = FSM_STATE_NONE;
        state_data->m_next_state     = FSM_STATE_NONE;

        app_data->m_state_fn[FSM_STATE_CONNECT_TO_WIFI]      = state_connect_to_wifi;
        app_data->m_state_fn[FSM_STATE_HANDLE_LITTLE_FS]     = state_handle_little_fs;
        app_data->m_state_fn[FSM_STATE_HANDLE_SD_CARD]       = state_handle_sd_card;
        app_data->m_state_fn[FSM_STATE_DOWNLOAD_ASSETS]      = state_download_assets;
        app_data->m_state_fn[FSM_STATE_VERIFY_ASSETS]        = state_verify_assets;
        app_data->m_state_fn[FSM_STATE_HANDLE_SENSOR_SERVER] = state_handle_sensor_server;
        app_data->m_state_fn[FSM_STATE_INITIALIZE_SENSORS]   = state_initialize_sensors;
        app_data->m_state_fn[FSM_STATE_INITIALIZE_DISPLAY]   = state_initialize_display;
        app_data->m_state_fn[FSM_STATE_SHOW_SPLASH_SCREEN]   = state_show_splash_screen;
        app_data->m_state_fn[FSM_STATE_INITIALIZE_TOUCH]     = state_initialize_touch;
        app_data->m_state_fn[FSM_STATE_INITIALIZE_SCRIPT_VM] = state_initialize_script_vm;
        app_data->m_state_fn[FSM_STATE_ACTIVE]               = state_active;
        app_data->m_state_fn[FSM_STATE_ERROR]                = state_error;
    }

    bool get_script(app_data_t* data, const void*& script_binary, u32& script_binary_size, u32& script_memory_capacity)
    {
        if (data != nullptr)
        {
            script_binary          = data->m_script_binary;
            script_binary_size     = data->m_script_binary_size;
            script_memory_capacity = data->m_script_binary_capacity;
        }
        return script_binary != nullptr && script_binary_size > 0;
    }

    void handle_state_data(fsm_state_data_t* state_data, app_data_t* app_data)
    {
        if (state_data->m_next_state != FSM_STATE_NONE)
        {
            state_data->m_previous_state = state_data->m_current_state;
            state_data->m_current_state  = state_data->m_next_state;
            state_data->m_next_state     = FSM_STATE_NONE;
            state_data->m_state_data     = 0;
        }
    }

    void update_touch(app_data_t& data, u64 now_ms)
    {
        ntouch::touch_point_t p1, p2;
        bool                  p1_present = false;
        bool                  p2_present = false;

        u8                    num_points = 0;
        ntouch::touch_point_t points[ntouch::TP_CT_MAX_TOUCH];
        if (ntouch::touch_scan(data.m_touch, now_ms, points, ntouch::TP_CT_MAX_TOUCH, &num_points) == true)
        {
            if (num_points > 0)
            {
                nlog::log_infof("main", "Touch detected: %u points", va_list_t(va_t(num_points)));
                for (u8 i = 0; i < num_points; i++)
                {
                    const ntouch::touch_point_t* point = &points[i];
                    if (point != nullptr)
                    {
                        if (i == 0)
                        {
                            p1_present = true;
                            p1         = *point;
                        }
                        else if (i == 1)
                        {
                            p2_present = true;
                            p2         = *point;
                        }
                    }
                }
            }

            ntouch::egesture_type_t gesture = ntouch::update_touch_gesture(data.m_touch_gesture, now_ms, p1_present, p1, p2_present, p2);

            if (gesture == ntouch::GT_TAP)
            {
                ntouch::touch_point_t tap_point = ntouch::get_single_tap_location(data.m_touch_gesture);
                nlog::println("Single tap detected");
            }
            else if (gesture == ntouch::GT_DOUBLE_TAP)
            {
                ntouch::touch_point_t tap_point = ntouch::get_single_tap_location(data.m_touch_gesture);
                nlog::println("Double tap detected");
            }
            else if ((gesture & ntouch::GT_SWIPE_ONE_FINGER) != 0)
            {
                if ((gesture & ntouch::GT_DIR_LEFT) != 0)
                    nlog::println("One finger swipe left detected");
                else if ((gesture & ntouch::GT_DIR_RIGHT) != 0)
                    nlog::println("One finger swipe right detected");
                else if ((gesture & ntouch::GT_DIR_UP) != 0)
                    nlog::println("One finger swipe up detected");
                else if ((gesture & ntouch::GT_DIR_DOWN) != 0)
                    nlog::println("One finger swipe down detected");
            }
            else if ((gesture & ntouch::GT_SWIPE_TWO_FINGER) != 0)
            {
                if ((gesture & ntouch::GT_DIR_LEFT) != 0)
                    nlog::println("Two finger swipe left detected");
                else if ((gesture & ntouch::GT_DIR_RIGHT) != 0)
                    nlog::println("Two finger swipe right detected");
                else if ((gesture & ntouch::GT_DIR_UP) != 0)
                    nlog::println("Two finger swipe up detected");
                else if ((gesture & ntouch::GT_DIR_DOWN) != 0)
                    nlog::println("Two finger swipe down detected");
            }
        }
    }

}  // namespace ncore