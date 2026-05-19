#include "rmui/c_display.h"
#include "cfenc/c_decoder.h"
#include "rcore/c_system.h"
#include "ccore/c_memory.h"

namespace ncore
{
    namespace nmui
    {
        void init(display_t* display, u16 display_width, u16 display_height)
        {
            display->m_width  = display_width;
            display->m_height = display_height;

            // use PSRAM to allocate the framebuffers since they are quite large
            // the tile buffer is small enough to fit in regular RAM
            display->m_fb = (u16*)nsystem::alloc_psram(display_width * display_height * sizeof(u16));
        }

        void render_tiles(display_t& display, const nfenc::dirty_tiles_t& dirty_tiles)
        {
            // Here we render the front buffer to the display, using the tile buffer to optimize rendering by only redrawing dirty tiles.
            const u16 num_h_tiles = dirty_tiles.m_tiles_rows;
            const u16 num_v_tiles = dirty_tiles.m_tiles_cols;
            const u16 tile_width  = 1 << dirty_tiles.m_tile_width_shift;

            for (u16 tile_y = 0; tile_y < num_v_tiles; ++tile_y)
            {
                const u32* dirty_tile_row = nfenc::get_tile_row_ptr(dirty_tiles, tile_y);

                for (u16 tile_x = 0; tile_x < num_h_tiles; ++tile_x)
                {
                    if (nfenc::is_tile_marked_on_row(dirty_tiles, dirty_tile_row, tile_x))
                    {
                        // Calculate the pixel coordinates for this tile
                        const u16 y_start      = tile_y << dirty_tiles.m_tile_height_shift;        // Convert tile_y to img_y
                        const u16 y_next_start = (tile_y + 1) << dirty_tiles.m_tile_height_shift;  // Convert tile_y to img_y
                        const u16 y_end        = (y_next_start < display.m_height) ? y_next_start : display.m_height;

                        const u16 x_start        = tile_x << dirty_tiles.m_tile_width_shift;  // Convert tile_x to img_x
                        const u16 width_in_bytes = sizeof(u16) * ((x_start + tile_width < display.m_width) ? tile_width : (display.m_width - x_start));

                        // TODO ====> Update this tile to the hardware display
                        for (u16 y = y_start; y < y_end; ++y)
                        {
                            u16* span = display.m_fb + (y * display.m_width + x_start);

                            // TODO copy to the actual LCD
                            u16* hwdisplay = 0;
                            g_memcpy(hwdisplay, span, width_in_bytes);
                        }
                    }
                }
            }
        }

    }  // namespace nmui
}  // namespace ncore
