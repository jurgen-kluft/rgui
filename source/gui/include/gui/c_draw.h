#ifndef __RGUI_DRAW_H__
#define __RGUI_DRAW_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    // We do need the state of all sensors etc...
    struct light_state_t
    {
        u32 m_hsv;         // HSV, H = 9 bits, S = 8 bits, V = 8 bits
        u8  m_on_off;      // On/Off state (0 = off, 1 = on)
        u8  m_warmth;      // Warmth level (0-255)
        u8  m_brightness;  // Brightness level (0-255)
        u8  m_reserved;    // Reserved for future use
    };

    struct environment_state_t
    {
        i8  m_temperature;  // Temperature reading in tenths of degrees Celsius
        u8  m_humidity;     // Humidity reading in tenths of percent
        u16 m_pressure;     // Pressure reading in tenths of hPa
        u16 m_co2_level;    // CO2 level reading in ppm
        u16 m_light_level;  // Light level reading in lux
    };

    struct home_state_t
    {
        char m_data[16]; // Date in MMM, DDD DD format (e.g. "Jan, Mon 01")
        char m_time[8];  // Time in HH:MM format (e.g. "13:37")

        environment_state_t m_outside_env;  // Temperature reading in tenths of degrees Celsius

        // entrance
        light_state_t m_entrance_light;

        // kitchen
        environment_state_t m_kitchen_env;  // Temperature reading in tenths of degrees Celsius
        light_state_t       m_kitchen_table_light;
        light_state_t       m_kitchen_ceiling_light;

        // bathroom
        environment_state_t m_bathroom_env;  // Temperature reading in tenths of degrees Celsius
        light_state_t       m_bathroom_ceiling_light_sink;
        light_state_t       m_bathroom_ceiling_light;

        // living room
        environment_state_t m_living_room_env;  // Temperature reading in tenths of degrees Celsius
        light_state_t       m_living_room_chandelier_light;
        light_state_t       m_living_room_left_stand_light;
        light_state_t       m_living_room_right_stand_light;
        light_state_t       m_living_room_ceiling_couch_light;
        light_state_t       m_living_room_ceiling_tv_light;
        light_state_t       m_living_room_piano_light;
        light_state_t       m_living_room_piano_ceiling_light;
        light_state_t       m_living_room_piano_chandelier_light;

        // hallway
        light_state_t m_hallway_painting_light;
        light_state_t m_hallway_ceiling_light;

        // sophia bedroom
        environment_state_t m_sophia_bedroom_env;  // Temperature reading in tenths of degrees Celsius
        light_state_t       m_sophia_bedroom_light;

        // jennifer bedroom
        environment_state_t m_jennifer_bedroom_env;  // Temperature reading in tenths of degrees Celsius
        light_state_t       m_jennifer_bedroom_light;

        // main bedroom
        environment_state_t m_main_bedroom_env;  // Temperature reading in tenths of degrees Celsius
        light_state_t       m_main_bedroom_light;
        light_state_t       m_main_left_stand_light;
        light_state_t       m_main_right_stand_light;
        light_state_t       m_main_ceiling_light;

        // main bathroom
        environment_state_t m_main_bathroom_env;  // Temperature reading in tenths of degrees Celsius
        light_state_t       m_main_bathroom_mirror_light;
        light_state_t       m_main_bathroom_ceiling_light;
    };

    namespace ngui
    {
        void init_renderer(u16 fb_width, u16 fb_height, u16 sram_canvas_height);

        void draw_begin_frame();
        void draw_end_frame();

        void clear_screen(u32 color);
        void draw_sprite(u16 spriteId, u16 x, u16 y);
        void draw_text(u8 fontId, const u8* text, u16 x, u16 y, u32 color);
        void draw_date(u8 fontId, u16 x, u16 y, u32 color);
        void draw_time(u8 fontId, u16 x, u16 y, u32 color);
        void draw_value(u8 fontId, i32 value, u8 unit, u16 x, u16 y, u32 color);
    }  // namespace ngui
}  // namespace ncore

#endif  // __RGUI_DRAW_H__