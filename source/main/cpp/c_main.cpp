#include "rcore/c_app.h"
#include "rcore/c_gpio.h"
#include "rcore/c_timer.h"
#include "rcore/c_log.h"
#include "rcore/c_packet.h"
#include "rcore/c_str.h"
#include "rcore/c_system.h"
#include "rcore/c_task.h"
#include "rcore/c_wire.h"
#include "ccore/c_random.h"

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"

#include "lib_wcs/c_lcd.h"
#include "lib_wcs/c_touch.h"
#include "lib_wcs/c_sdcard.h"

#include "rmui/c_mui_client.h"
#include "rmui/c_touch_gesture.h"

namespace ncore
{
    struct state_app_t
    {
        xor_random_t          gRandom;        // XORShift random number generator for any randomization needs
        ntouch::touch_panel_t gTouchPanel;    // Touch panel state
        nmui::touch_gesture_t gTouchGesture;  // Touch gesture state

        nwifi::wifi_manager_t gWifiManager;  // WiFi manager for network connectivity
        nwifi::wifi_config_t  gWifiConfig;   // WiFi configuration

        ntcp::tcp_client_t gSensorServerTcpClient;  // TCP client for server communication
        ntcp::config_t     gSensorServerTcpConfig;  // TCP client configuration
    };

    state_app_t  gAppState;
    state_task_t gAppTask;

}  // namespace ncore

namespace ncore
{
    namespace napp
    {
        void wakeup(state_t* state, ncore::nwakeup::reason_t reason)
        {
            // Handle wakeup reasons if needed (e.g., from deep sleep)
        }

        void presetup(state_t* state)
        {
            // Time critical setup before WiFi and other components are initialized can be done here
        }

        ntimer::periodic_task_t gBlinkLedTask = {1000, 0, nullptr, [](void* user) { nlcd::led_toggle(); }};

        void setup(state_t* state)
        {
            nlog::println("Setup begin");

            void* tcp_socket = ntcp::setup_default(&gAppState.gSensorServerTcpConfig);
            ntcp::setup(gAppState.gSensorServerTcpClient, &gAppState.gSensorServerTcpConfig, tcp_socket, SENSOR_SERVER_IP(), SENSOR_SERVER_TCPPORT(), nullptr, 0);

            nwifi::init_wifi_config(gAppState.gWifiConfig, WIFI_SSID(), WIFI_PASSWORD());
            nwifi::setup(gAppState.gWifiManager, &gAppState.gWifiConfig);
            nwifi::activate(gAppState.gWifiManager);

            if (nlcd::initialize() == false)
            {
                nlog::println("Failed to initialize LCD");
            }
            else
            {
                if (ntouch::tp_init(gAppState.gTouchPanel, nlcd::width(), nlcd::height()) == false)
                {
                    nlog::println("Failed to initialize touch panel");
                }
                nmui::init_touch_gesture(gAppState.gTouchGesture, nmui::gesture_config_t());

                if (nlcd::sdcard_initialize() == false)
                {
                    nlog::println("Failed to initialize SD card");
                }
                else
                {
                    nlog::println("SD card initialized successfully");

                    u64 total_bytes, free_bytes;
                    if (nlcd::sdcard_get_usage(&total_bytes, &free_bytes))
                    {
                        nlog::printfln("SD card total size: %.2f MB", va_t(total_bytes / (1024.0 * 1024.0)));
                        nlog::printfln("SD card free space: %.2f MB", va_t(free_bytes / (1024.0 * 1024.0)));
                    }
                }

                nmui::init_mui_client(&gAppState.gWifiManager);

                nlog::println("Setup complete");
            }
        }

        static u64 toggle_lcd_fill_time = 0;

        void tick(state_t* state)
        {
            const u64 now_ms = ntimer::millis();

            ntcp::tick_tcp_client(&gAppState.gWifiManager, gAppState.gSensorServerTcpClient);
            nmui::mui_state_t mui_state = nmui::tick_mui_client(&gAppState.gWifiManager, now_ms);

            if (mui_state == nmui::MUI_STATE_DISPLAY_OFF || mui_state == nmui::MUI_STATE_ACTIVE)
            {
                // When the display is OFF we can sample the sensors at the normal rate and send data
                // to the sensor server, but no frames will be requested from the server.
                // When the display is ACTIVE we will sample the sensors at a lower rate and send data
                // to the sensor server, and we will also request frames from the server at a regular interval.
                // When the display is RECEIVING_FRAME or RENDERING_FRAME this code here is not executed

                // Sample sensors
            }

            ntimer::tick_periodic_task(&gBlinkLedTask, now_ms);

            if (ntouch::tp_scan(gAppState.gTouchPanel, 0))
            {
                bool                p1_present = false;
                nmui::touch_point_t p1;
                bool                p2_present = false;
                nmui::touch_point_t p2;

                const u8 num_points = ntouch::tp_get_touch_point_num(gAppState.gTouchPanel);
                for (u8 i = 0; i < num_points; i++)
                {
                    if (ntouch::tp_is_valid_touch_point(gAppState.gTouchPanel, i))
                    {
                        if (i == 0)
                        {
                            p1_present = true;
                            p1.m_x     = ntouch::tp_get_touch_point(gAppState.gTouchPanel, i).x;
                            p1.m_y     = ntouch::tp_get_touch_point(gAppState.gTouchPanel, i).y;
                        }
                        else if (i == 1)
                        {
                            p2_present = true;
                            p2.m_x     = ntouch::tp_get_touch_point(gAppState.gTouchPanel, i).x;
                            p2.m_y     = ntouch::tp_get_touch_point(gAppState.gTouchPanel, i).y;
                        }
                    }
                }

                nmui::gesture_type_t gesture = nmui::update_touch_gesture(gAppState.gTouchGesture, p1_present, p1, p2_present, p2);

                if (gesture == nmui::GT_TAP)
                {
                    nmui::touch_point_t tap_point = nmui::get_single_tap_location(gAppState.gTouchGesture);
                    u8                  event_data[16];
                    const u16           event_data_len = nmui::write_single_tap_event(event_data, (s16)tap_point.m_x, (s16)tap_point.m_y);
                    nmui::send_input_event(event_data, event_data_len);
                }
                else if (gesture == nmui::GT_DOUBLE_TAP)
                {
                    nmui::touch_point_t tap_point = nmui::get_single_tap_location(gAppState.gTouchGesture);
                    u8                 event_data[16];
                    const u16          event_data_len = nmui::write_double_tap_event(event_data, (s16)tap_point.m_x, (s16)tap_point.m_y);
                    nmui::send_input_event(event_data, event_data_len);
                }
                else if ((gesture & nmui::GT_SWIPE_ONE_FINGER) != 0)
                {
                    if ((gesture & nmui::GT_DIR_LEFT) != 0)
                        nlog::println("One finger swipe left detected");
                    else if ((gesture & nmui::GT_DIR_RIGHT) != 0)
                        nlog::println("One finger swipe right detected");
                    else if ((gesture & nmui::GT_DIR_UP) != 0)
                        nlog::println("One finger swipe up detected");
                    else if ((gesture & nmui::GT_DIR_DOWN) != 0)
                        nlog::println("One finger swipe down detected");
                }
                else if ((gesture & nmui::GT_SWIPE_TWO_FINGER) != 0)
                {
                    if ((gesture & nmui::GT_DIR_LEFT) != 0)
                        nlog::println("Two finger swipe left detected");
                    else if ((gesture & nmui::GT_DIR_RIGHT) != 0)
                        nlog::println("Two finger swipe right detected");
                    else if ((gesture & nmui::GT_DIR_UP) != 0)
                        nlog::println("Two finger swipe up detected");
                    else if ((gesture & nmui::GT_DIR_DOWN) != 0)
                        nlog::println("Two finger swipe down detected");
                }
            }

            if (ntimer::millis() - toggle_lcd_fill_time > 2000)
            {
                nlog::println("Filling random area with random color");

                // Fill a random area with a random color every 2 seconds
                u16 color = (u16)g_random_u32(&gAppState.gRandom, 16);  // Random RGB565 color
                u16 sx    = (u16)g_random_u32_range(&gAppState.gRandom, 0, nlcd::width() - 51);
                u16 sy    = (u16)g_random_u32_range(&gAppState.gRandom, 0, nlcd::height() - 51);
                u16 ex    = sx + (g_random_u32_range(&gAppState.gRandom, 0, 40) + 10);  // Random width between 10 and 50 pixels
                u16 ey    = sy + (g_random_u32_range(&gAppState.gRandom, 0, 40) + 10);  // Random height between 10 and 50 pixels

                nlcd::draw_rectangle(sx, sy, ex, ey, color);
                toggle_lcd_fill_time = ntimer::millis();
            }
        }

    }  // namespace napp
}  // namespace ncore