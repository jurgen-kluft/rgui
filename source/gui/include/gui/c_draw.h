#ifndef __RGUI_DRAW_H__
#define __RGUI_DRAW_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace ngx2
    {
        struct sprite_pack_t;
        struct font_pack_t;
        struct palette_pack_t;
    }  // namespace ngx2

    namespace ngui
    {
        void init_renderer(u16 fb_width, u16 fb_height, u16 sram_canvas_height, ngx2::sprite_pack_t* sprite_pack, ngx2::font_pack_t* font_pack, ngx2::palette_pack_t* palette_pack);

        void draw_begin_frame();
        void draw_end_frame();

        void clear_background(u32 color);
        void draw_background(u16 spriteId);

        void draw_sprite(u16 spriteId, u16 x, u16 y);
        void draw_sprite(u16 spriteId, u16 x, u16 y, u16 w, u16 h);

        void draw_text(u8 fontId, u16 size, const u8* text, u16 x, u16 y, u32 color);
        void draw_date(u8 fontId, u16 size, u16 x, u16 y, u32 color, u16 year, u8 month, u8 day, u8 day_of_week);
        void draw_time(u8 fontId, u16 size, u16 x, u16 y, u32 color, u8 hour, u8 minute, u8 second);
        void draw_value(u8 fontId, u16 size, i32 value, u8 unit, u16 x, u16 y, u32 color);
    }  // namespace ngui
}  // namespace ncore

#endif  // __RGUI_DRAW_H__