#ifndef __RGUI_ASSET_DB_H__
#define __RGUI_ASSET_DB_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "cgx2/c_types.h"

namespace ncore
{
    namespace ngui
    {
        void set_sprite_pack(ngx2::sprite_pack_t* sprite_pack);
        void set_palette_pack(ngx2::palette_pack_t* palette_pack);
        void set_font_pack(ngx2::font_pack_t* font_pack);
        void set_script_binary(const void* script_binary, u32 script_binary_size);

        ngx2::sprite_t*  get_sprite(u32 index);
        ngx2::palette_t* get_palette(u32 index);
        ngx2::font_t*    get_font(u32 index);
        const void*      get_script_binary(u32* script_binary_size);

    }  // namespace ngui
}  // namespace ncore

#endif  // __RGUI_ASSET_DB_H__