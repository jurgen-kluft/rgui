#ifndef __RGUI_DISPLAY_H__
#define __RGUI_DISPLAY_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace ngui
    {
        struct display_t
        {
            u16  m_width;   // width of the framebuffer in pixels
            u16  m_height;  // height of the framebuffer in pixels
            u16* m_fb;      // RGB565 pixel data for the framebuffer
        };

        void init_display(display_t& display, u16 display_width, u16 display_height);

    }  // namespace ngui
}  // namespace ncore

#endif  // __RGUI_DISPLAY_H__
