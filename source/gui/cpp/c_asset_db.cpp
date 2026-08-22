#include "ccore/c_memory.h"
#include "ccore/c_random.h"
#include "ccore/c_memory.h"
#include "rcore/c_system.h"

#include "gui/c_asset_db.h"

namespace ncore
{
    namespace ngui
    {
        static ngx2::sprite_pack_t*  s_sprites  = nullptr;  // pointer to the array of assets in PSRAM
        static ngx2::font_pack_t*    s_fonts    = nullptr;  // pointer to the array of fonts in PSRAM
        static ngx2::palette_pack_t* s_palettes = nullptr;  // pointer to the array of palettes in PSRAM

        void set_sprite_pack(ngx2::sprite_pack_t* sprite_pack) { s_sprites = sprite_pack; }
        void set_palette_pack(ngx2::palette_pack_t* palette_pack) { s_palettes = palette_pack; }
        void set_font_pack(ngx2::font_pack_t* font_pack) { s_fonts = font_pack; }

        ngx2::sprite_t* get_sprite(u32 index)
        {
            ngx2::sprite_t* sprite = nullptr;
            if (s_sprites != nullptr && index < s_sprites->num_sprites)
                sprite = s_sprites->sprites.item<ngx2::sprite_t>(index);
            return sprite;
        }

        ngx2::palette_t* get_palette(u32 index)
        {
            ngx2::palette_t* palette = nullptr;
            if (s_palettes != nullptr && index < s_palettes->num_palettes)
                palette = s_palettes->palettes.item<ngx2::palette_t>(index);
            return palette;
        }

        ngx2::font_t* get_font(u32 index)
        {
            ngx2::font_t* font = nullptr;
            if (s_fonts != nullptr && index < s_fonts->num_fonts)
                font = s_fonts->fonts.item<ngx2::font_t>(index);
            return font;
        }

    }  // namespace ngui
}  // namespace ncore