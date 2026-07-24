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
        // The asset db exists in PSRAM
        void prepare_asset_db(u32 size = 2 * 1024 * 1024);  // default size is 2MB
        void build_asset_db(byte const* data, u32 size, u32 offset);
        void finalize_asset_db();

        ngx2::sprite_t*  get_sprite(u32 index);
        ngx2::palette_t* get_palette(u32 index);
        ngx2::font_t*    get_font(u32 index);

    }  // namespace ngui
}  // namespace ncore

#endif  // __RGUI_ASSET_DB_H__