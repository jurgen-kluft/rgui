#include "rcore/c_app.h"
#include "rcore/c_gpio.h"
#include "rcore/c_timer.h"
#include "rcore/c_log.h"
#include "rcore/c_packet.h"
#include "rcore/c_str.h"
#include "rcore/c_system.h"
#include "rcore/c_wire.h"
#include "ccore/c_random.h"

#include "rwifi/c_wifi_mgr.h"
#include "rwifi/c_tcp_client.h"

#include "lib_guition/c_lcd.h"
#include "lib_guition/c_sdcard.h"

#include "lib_touch/c_touch_gt911.h"
#include "lib_touch/c_touch_gesture.h"

#include "gui/c_display.h"
#include "gui/c_draw.h"
#include "gui/c_asset_db.h"

namespace ncore
{
    struct state_app_t
    {
        xor_random_t            gRandom;        // XORShift random number generator for any randomization needs
        ntouch::touch_t         gTouch;         // Touch
        ntouch::touch_gesture_t gTouchGesture;  // Touch gesture state

        nwifi::wifi_manager_t gWifiManager;  // WiFi manager for network connectivity
        nwifi::wifi_config_t  gWifiConfig;   // WiFi configuration

        ntcp::tcp_client_t gSensorServerTcpClient;  // TCP client for server communication
        ntcp::config_t     gSensorServerTcpConfig;  // TCP client configuration
    };

    state_app_t gAppState;

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
                const u8                i2c_addr = 0x5D;                     // GT911 I2C address
                const i8                sda_pin  = 19;                       // GT911 SDA pin
                const i8                scl_pin  = 45;                       // GT911 SCL pin
                const i8                int_pin  = -1;                       // GT911 INT pin (not used)
                const i8                rst_pin  = -1;                       // GT911 RST pin (not used)
                const ntouch::erotate_t rotate   = ntouch::cTP_ROTATE_0;     // No rotation
                const ntouch::emirror_t mirror   = ntouch::cTP_MIRROR_NONE;  // No mirroring

                const u16 width  = nlcd::width();
                const u16 height = nlcd::height();

                if (ntouch::ngt911::touch_init(gAppState.gTouch, width, height, 50, i2c_addr, sda_pin, scl_pin, int_pin, rst_pin, rotate, mirror) == false)
                {
                    nlog::println("Failed to initialize touch panel");
                }
                ntouch::init_touch_gesture(gAppState.gTouchGesture, ntouch::gesture_config_t());

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

                nlog::println("Setup complete");
            }
        }

        static u64 toggle_lcd_fill_time = 0;

        void tick(state_t* state)
        {
            const u64 now_ms = ntimer::millis();

            ntcp::tick_tcp_client(&gAppState.gWifiManager, gAppState.gSensorServerTcpClient);

            {
                // When the display is OFF we can sample the sensors at the normal rate and send data
                // to the sensor server, but no frames will be requested from the server.
                // When the display is ACTIVE we will sample the sensors at a lower rate and send data
                // to the sensor server, and we will also request frames from the server at a regular interval.
                // When the display is RECEIVING_FRAME or RENDERING_FRAME this code here is not executed

                // Sample sensors
            }

            ntimer::tick_periodic_task(&gBlinkLedTask, now_ms);

            ntouch::touch_point_t p1, p2;
            bool                  p1_present = false;
            bool                  p2_present = false;

            u8                    num_points = 0;
            ntouch::touch_point_t points[ntouch::TP_CT_MAX_TOUCH];
            if (ntouch::touch_scan(gAppState.gTouch, now_ms, points, ntouch::TP_CT_MAX_TOUCH, &num_points) == true)
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

                ntouch::egesture_type_t gesture = ntouch::update_touch_gesture(gAppState.gTouchGesture, now_ms, p1_present, p1, p2_present, p2);

                if (gesture == ntouch::GT_TAP)
                {
                    ntouch::touch_point_t tap_point = ntouch::get_single_tap_location(gAppState.gTouchGesture);
                    nlog::println("Single tap detected");
                }
                else if (gesture == ntouch::GT_DOUBLE_TAP)
                {
                    ntouch::touch_point_t tap_point = ntouch::get_single_tap_location(gAppState.gTouchGesture);
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