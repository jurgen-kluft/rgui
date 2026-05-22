#ifndef __RMUI_DISPLAY_H__
#define __RMUI_DISPLAY_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "cfenc/c_dirty_tiles.h"

namespace ncore
{
    namespace nmui
    {
        struct display_t
        {
            u16* m_fb;      // RGB565 pixel data for the framebuffer
            u16  m_width;   // width of the framebuffer in pixels
            u16  m_height;  // height of the framebuffer in pixels
        };

        void init(display_t* display, u16 display_width, u16 display_height);
        
        // Simple tile based rendering function that takes the dirty tiles and updates 
        // only those tiles on the hardware display.
        void render_tiles(display_t& display, const nfenc::dirty_tiles_t& dirty_tiles);


    }  // namespace nmui
}  // namespace ncore

#endif  // __RMUI_DISPLAY_H__
