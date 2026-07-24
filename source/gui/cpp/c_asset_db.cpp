#include "ccore/c_memory.h"
#include "ccore/c_random.h"
#include "ccore/c_memory.h"
#include "rcore/c_system.h"

#include "gui/c_asset_db.h"

namespace ncore
{
    namespace ngui
    {
        struct asset_db_t
        {
            u32              m_num_sprites;   // number of assets in the registry
            u32              m_num_fonts;     // number of fonts in the registry
            u32              m_num_palettes;  // number of palettes in the registry
            ngx2::sprite_t*  m_sprites;       // pointer to the array of assets in PSRAM
            ngx2::font_t*    m_fonts;         // pointer to the array of fonts in PSRAM
            ngx2::palette_t* m_palettes;      // pointer to the array of palettes in PSRAM
        };

        static u8*        s_asset_data_arena      = nullptr;  // pointer to the asset data arena in PSRAM
        static u32        s_asset_data_arena_size = 0;        // size of the asset data arena in bytes
        static asset_db_t s_asset_db;                         // asset database containing the assets in PSRAM

        // Initialize the asset registry and allocate the asset data arena in PSRAM
        // Note: We don't allow the asset data arena to be resized after it has been allocated,
        //       so we need to know the total size of the asset database in advance.
        void prepare_asset_db(u32 size)
        {
            if (s_asset_data_arena == nullptr)
            {
                s_asset_data_arena      = nsystem::alloc_psram(size);
                s_asset_data_arena_size = size;
            }
            g_memset(&s_asset_db, 0, sizeof(asset_db_t));
        }

        // We are receiving the asset database over WiFi, block by block, and we need to copy it to PSRAM
        void build_asset_db(byte const* data, u32 size, u32 offset)
        {
            if (s_asset_data_arena != nullptr && (offset + size) <= s_asset_data_arena_size)
            {
                u8* asset_data_ptr = s_asset_data_arena + offset;
                g_memcpy(asset_data_ptr, data, size);
            }
        }

        void finalize_asset_db()
        {
            // TODO, parse the asset database
        }

        ngx2::sprite_t* get_sprite(u32 index)
        {
            ngx2::sprite_t* sprite = nullptr;
            if (s_asset_db.m_sprites != nullptr && index < s_asset_db.m_num_sprites)
                sprite = &s_asset_db.m_sprites[index];
            return sprite;
        }

        ngx2::palette_t* get_palette(u32 index)
        {
            ngx2::palette_t* palette = nullptr;
            if (s_asset_db.m_palettes != nullptr && index < s_asset_db.m_num_palettes)
                palette = &s_asset_db.m_palettes[index];
            return palette;
        }

        ngx2::font_t* get_font(u32 index)
        {
            ngx2::font_t* font = nullptr;
            if (s_asset_db.m_fonts != nullptr && index < s_asset_db.m_num_fonts)
                font = &s_asset_db.m_fonts[index];
            return font;
        }

    }  // namespace ngui
}  // namespace ncore