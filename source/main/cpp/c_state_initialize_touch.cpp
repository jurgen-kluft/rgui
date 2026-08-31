#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "lib_guition/c_lcd.h"

#include "lib_touch/c_touch_gt911.h"
#include "lib_touch/c_touch_gesture.h"

#include "main/c_app_data.h"

namespace ncore
{
    void state_initialize_touch(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms)
    {
        if (state_data.m_state_data == 0)
        {
            nlog::println("Initialize Touch - Begin");

            const u8                i2c_addr = 0x5D;                     // GT911 I2C address
            const i8                sda_pin  = 19;                       // GT911 SDA pin
            const i8                scl_pin  = 45;                       // GT911 SCL pin
            const i8                int_pin  = -1;                       // GT911 INT pin (not used)
            const i8                rst_pin  = -1;                       // GT911 RST pin (not used)
            const ntouch::erotate_t rotate   = ntouch::cTP_ROTATE_0;     // No rotation
            const ntouch::emirror_t mirror   = ntouch::cTP_MIRROR_NONE;  // No mirroring

            const u16 width  = nlcd::width();
            const u16 height = nlcd::height();

            if (ntouch::ngt911::touch_init(app_data.m_touch, width, height, 50, i2c_addr, sda_pin, scl_pin, int_pin, rst_pin, rotate, mirror) == false)
            {
                nlog::println("Initialize Touch - Failed to initialize touch panel");
                to_state_error(state_data);
                return;
            }

            ntouch::init_touch_gesture(app_data.m_touch_gesture, ntouch::gesture_config_t());

            // Leave
            to_state_next(state_data);

            nlog::println("Initialize Touch -  Complete");
        }
    }
}  // namespace ncore
