#include "rcore/c_system.h"
#include "ccore/c_memory.h"

#include "gui/c_display.h"

namespace ncore
{
    namespace ngui
    {
        void init_display(display_t& display, u16 display_width, u16 display_height)
        {
            display.m_width  = display_width;
            display.m_height = display_height;

            // use PSRAM to allocate the framebuffers since they are quite large
            // the tile buffer is small enough to fit in regular RAM
            display.m_fb = (u16*)nsystem::alloc_psram_aligned(display_width * display_height * sizeof(u16), 64);
        }
    }  // namespace ngui
}  // namespace ncore
