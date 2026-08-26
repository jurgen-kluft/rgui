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

#include "lib_guition/c_lcd.h"
#include "lib_guition/c_sdcard.h"

#include "lib_touch/c_touch_gt911.h"
#include "lib_touch/c_touch_gesture.h"

#include "gui/c_display.h"
#include "gui/c_draw.h"

#include "main/c_app_data.h"

namespace ncore
{
    void init_app_data(app_data_t& app_data)
    {
        g_memclr(&app_data, sizeof(app_data_t));

        app_data.m_random.reset(0x1234567890abcdef);  // Initialize the random number generator with a fixed seed for reproducibility

        ntouch::gesture_config_t gesture_config;
        ntouch::init_touch_gesture(app_data.m_touch_gesture, gesture_config);

        nnet::init_wifi_config(app_data.m_wifi_config, WIFI_SSID(), WIFI_PASSWORD(), 1000, 16000, 2.0f, 0.1f);
        nnet::setup_default(&app_data.m_tcpclient_config);

        app_data.m_state_data.m_state_data     = 0;
        app_data.m_state_data.m_current_state  = FSM_STATE_CONNECT_TO_WIFI;
        app_data.m_state_data.m_previous_state = FSM_STATE_NONE;
        app_data.m_state_data.m_next_state     = FSM_STATE_NONE;

        app_data.m_state_fn[FSM_STATE_CONNECT_TO_WIFI]              = state_connect_to_wifi;
        app_data.m_state_fn[FSM_STATE_DOWNLOAD_FROM_ASSET_SERVER]   = state_download_from_asset_server;
        app_data.m_state_fn[FSM_STATE_CONNECT_TO_SENSOR_SERVER]     = state_connect_to_sensor_server;
        app_data.m_state_fn[FSM_STATE_INITIALIZE_SENSORS]           = state_initialize_sensors;
        app_data.m_state_fn[FSM_STATE_INITIALIZE_DISPLAY]           = state_initialize_display;
        app_data.m_state_fn[FSM_STATE_SHOW_SPLASH_SCREEN]           = state_show_splash_screen;
        app_data.m_state_fn[FSM_STATE_INITIALIZE_TOUCH]             = state_initialize_touch;
        app_data.m_state_fn[FSM_STATE_INITIALIZE_SDCARD]            = state_initialize_sdcard;
        app_data.m_state_fn[FSM_STATE_INITIALIZE_SCRIPT_VM]         = state_initialize_script_vm;
        app_data.m_state_fn[FSM_STATE_ACTIVE_DISPLAY_ON]            = state_active_display_on;
        app_data.m_state_fn[FSM_STATE_ACTIVE_DISPLAY_OFF]           = state_active_display_off;
        app_data.m_state_fn[FSM_STATE_ERROR]                        = state_error;
    }

    ngx2::sprite_t* get_sprite(app_data_t* data, u32 index)
    {
        ngx2::sprite_t* sprite = nullptr;
        if (data->m_sprites != nullptr && index < data->m_sprites->num_sprites)
            sprite = data->m_sprites->sprites.item<ngx2::sprite_t>(index);
        return sprite;
    }

    ngx2::palette_t* get_palette(app_data_t* data, u32 index)
    {
        ngx2::palette_t* palette = nullptr;
        if (data->m_palettes != nullptr && index < data->m_palettes->num_palettes)
            palette = data->m_palettes->palettes.item<ngx2::palette_t>(index);
        return palette;
    }

    ngx2::font_t* get_font(app_data_t* data, u32 index)
    {
        ngx2::font_t* font = nullptr;
        if (data->m_fonts != nullptr && index < data->m_fonts->num_fonts)
            font = data->m_fonts->fonts.item<ngx2::font_t>(index);
        return font;
    }

    bool get_script(app_data_t* data, const void*& script_binary, u32& script_binary_size)
    {
        if (data != nullptr)
        {
            script_binary      = data->m_script_binary;
            script_binary_size = data->m_script_binary_size;
        }
        return script_binary != nullptr && script_binary_size > 0;
    }

    void handle_state_data(fsm_state_data_t& state_data, app_data_t& app_data)
    {
        if (state_data.m_next_state != FSM_STATE_NONE)
        {
            state_data.m_previous_state = state_data.m_current_state;
            state_data.m_current_state  = state_data.m_next_state;
            state_data.m_next_state     = FSM_STATE_NONE;
            state_data.m_state_data     = 0;
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