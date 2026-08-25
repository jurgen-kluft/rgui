#ifndef __RGUI_MAIN_H__
#define __RGUI_MAIN_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "ccore/c_random.h"

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"

#include "lib_guition/c_lcd.h"
#include "lib_guition/c_sdcard.h"

#include "lib_touch/c_touch_gt911.h"
#include "lib_touch/c_touch_gesture.h"


namespace ncore
{
    struct app_data_t
    {
        xor_random_t            gRandom;        // XORShift random number generator for any randomization needs
        ntouch::touch_t         gTouch;         // Touch
        ntouch::touch_gesture_t gTouchGesture;  // Touch gesture state

        nnet::wifi_manager_t gWifiManager;  // WiFi manager for network connectivity
        nnet::wifi_config_t  gWifiConfig;   // WiFi configuration

        nnet::tcp_client_t gSensorServerTcpClient;  // TCP client for server communication
        nnet::config_t     gSensorServerTcpConfig;  // TCP client configuration
    };

    extern app_data_t gAppState;

    enum app_state_t
    {
        APP_STATE_CONNECT_TO_ASSET_SERVER = 0,
        APP_STATE_DOWNLOAD_FROM_ASSET_SERVER,
        APP_STATE_VALIDATE_ASSET_DATA,
        APP_STATE_DISCONNECT_FROM_ASSET_SERVER,
        APP_STATE_CONNECT_TO_SENSOR_SERVER,
        APP_STATE_INITIALIZE_SENSORS,
        APP_STATE_INITIALIZE_DISPLAY,
        APP_STATE_SHOW_SPLASH_SCREEN,
        APP_STATE_INITIALIZE_TOUCH,
        APP_STATE_INITIALIZE_SDCARD,
        APP_STATE_INITIALIZE_SCRIPT_VM,
        APP_STATE_ACTIVE_DISPLAY_ON,
        APP_STATE_ACTIVE_DISPLAY_OFF,
        APP_STATE_ERROR
    };

    enum state_call_t
    {
        STATE_CALL_ENTER = 0,
        STATE_CALL_UPDATE,
        STATE_CALL_LEAVE
    };

    i32 state_connect_to_asset_server(state_call_t call, app_data_t& app_data);
    i32 state_download_from_asset_server(state_call_t call, app_data_t& app_data);
    i32 state_validate_asset_data(state_call_t call, app_data_t& app_data);
    i32 state_disconnect_from_asset_server(state_call_t call, app_data_t& app_data);

    i32 state_connect_to_sensor_server(state_call_t call, app_data_t& app_data);
    i32 state_initialize_sensors(state_call_t call, app_data_t& app_data);

    i32 state_initialize_display(state_call_t call, app_data_t& app_data);
    i32 state_show_splash_screen(state_call_t call, app_data_t& app_data);
    i32 state_initialize_touch(state_call_t call, app_data_t& app_data);
    i32 state_initialize_sdcard(state_call_t call, app_data_t& app_data);

    i32 state_initialize_script_vm(state_call_t call, app_data_t& app_data);

    i32 state_active_display_on(state_call_t call, app_data_t& app_data);
    i32 state_active_display_off(state_call_t call, app_data_t& app_data);

    i32 state_error(state_call_t call, app_data_t& app_data);

}  // namespace ncore

#endif  // __RGUI_MAIN_H__
