#include "ccore/c_memory.h"
#include "ccore/c_printf.h"
#include "rcore/c_system.h"

#include "gui/c_asset_db.h"
#include "gui/c_draw.h"

namespace ncore
{
    home_state_t g_home_state;

    namespace ngui
    {
        enum fb_state_t
        {
            FB_IDLE,  // Buffer is ready to be written to
            FB_READY  // Buffer is fully rendered, waiting for the next VSYNC swap
        };

        typedef byte draw_cmd_t;

        const draw_cmd_t DRAW_CMD_NOP        = 0;
        const draw_cmd_t DRAW_CMD_CLR_SCREEN = 1;
        const draw_cmd_t DRAW_CMD_SPRITE     = 2;
        const draw_cmd_t DRAW_CMD_TEXT       = 3;
        const draw_cmd_t DRAW_CMD_VALUE      = 4;

        struct draw_clr_screen_args_t
        {
            u16 color;  // Color to fill the screen with
        };

        struct draw_sprite_args_t
        {
            u16 x;              // X coordinate
            u16 y;              // Y coordinate
            u16 sprite_index;   // Index of the sprite in the asset database
            u16 palette_index;  // Index of the palette in the asset database
            u8  blend_alpha;    // Alpha value for blending (0-255)
            u8  reserved;       // Reserved for future use
        };

        struct draw_text_args_t
        {
            u16         x;           // X coordinate
            u16         y;           // Y coordinate
            u16         color;       // Color of the text
            u16         font_index;  // Index of the font in the asset database
            const char* text;        // Pointer to the text string to be drawn
        };

        struct draw_value_args_t
        {
            u16 x;           // X coordinate
            u16 y;           // Y coordinate
            u16 color;       // Color of the value text
            u8  font_index;  // Index of the font in the asset database
            u8  unit;        // Unit of the value (e.g., 0 for none, 1 for px, etc.)
            i32 value;       // Integer value to be drawn
        };

        union draw_cmd_args_t
        {
            draw_clr_screen_args_t clr_screen_args;
            draw_sprite_args_t     sprite_args;
            draw_text_args_t       text_args;
            draw_value_args_t      value_args;
        };

        struct renderer_t
        {
            volatile fb_state_t m_fb_state;
            u16                 m_fb_width;
            u16                 m_fb_height;
            u16*                m_fb[2];
            u16                 m_current_fb;
            u16                 m_cmd_count;
            u16*                m_sram_canvas;
            u16                 m_sram_canvas_height;
            u16                 m_sram_canvas_width;
            i32                 m_value_str_cursor;
            char*               m_value_str_buffer;
            draw_cmd_t*         m_cmd_buffer;
            draw_cmd_args_t*    m_cmd_args;
        };

        const static i32 g_max_cmd_count         = 256;     // Maximum number of draw commands per frame
        const static i32 g_value_str_buffer_size = 16 * 8;  // Buffer for multiple value strings representation

        static renderer_t g_renderer;

        void init_renderer(u16 fb_width, u16 fb_height, u16 sram_canvas_height)
        {
            g_renderer.m_fb_state           = FB_IDLE;
            g_renderer.m_fb_width           = fb_width;
            g_renderer.m_fb_height          = fb_height;
            g_renderer.m_current_fb         = 0;
            g_renderer.m_cmd_count          = 0;
            g_renderer.m_sram_canvas_height = sram_canvas_height;
            g_renderer.m_sram_canvas_width  = fb_width;

            // Allocate SRAM canvas
            g_renderer.m_sram_canvas = (u16*)nsystem::malloc(sram_canvas_height * fb_width * sizeof(u16));

            // Allocate command buffer and arguments
            g_renderer.m_cmd_buffer = (draw_cmd_t*)nsystem::malloc(g_max_cmd_count * sizeof(draw_cmd_t));
            g_renderer.m_cmd_args   = (draw_cmd_args_t*)nsystem::malloc(g_max_cmd_count * sizeof(draw_cmd_args_t));

            g_renderer.m_value_str_buffer = (char*)nsystem::malloc(g_value_str_buffer_size);  // Buffer for multiple value strings representation
        }

        void draw_begin_frame()
        {
            renderer_t& r = g_renderer;

            if (r.m_fb_state == FB_IDLE)
            {
                r.m_cmd_count        = 0;
                r.m_value_str_cursor = 0;  // Reset the cursor for the value string buffer
            }
        }

        void draw_end_frame()
        {
            renderer_t& r = g_renderer;

            // Skip entirely if the previous frame hasn't been picked up by the VSYNC interrupt yet
            if (r.m_fb_state != FB_IDLE)
            {
                return;
            }

            const i16 num_slices = (r.m_fb_height + r.m_sram_canvas_height - 1) / r.m_sram_canvas_height;
            for (i16 i = 0; i < num_slices; i++)
            {
                const i16 slice_y_start = i * r.m_sram_canvas_height;
                const i16 slice_y_end   = slice_y_start + r.m_sram_canvas_height - 1;

                g_memclr(r.m_sram_canvas, r.m_sram_canvas_height * r.m_sram_canvas_width * sizeof(u16));
                for (i16 c = 0; c < r.m_cmd_count; c++)
                {
                    draw_cmd_t& cmd = r.m_cmd_buffer[c];
                    if (cmd == DRAW_CMD_NOP)
                        continue;

                    const draw_cmd_args_t& args   = r.m_cmd_args[c];
                    intersect_result_t     result = intersects_slice(r, cmd, &args, slice_y_start, slice_y_end);
                    if (result == INTERSECT_NO)
                        continue;

                    switch (cmd)
                    {
                        case DRAW_CMD_CLR_SCREEN:
                            {
                                draw_clr_screen_args_t* clr_args = (draw_clr_screen_args_t*)&args.clr_screen_args;
                                g_memset(r.m_sram_canvas, clr_args->color, r.m_sram_canvas_height * r.m_sram_canvas_width * sizeof(u16));
                                break;
                            }
                        case DRAW_CMD_SPRITE:
                            {
                                draw_sprite_args_t* sprite_args = (draw_sprite_args_t*)&args.sprite_args;
                                //
                                break;
                            }
                        case DRAW_CMD_TEXT:
                            {
                                draw_text_args_t* text_args = (draw_text_args_t*)&args.text_args;
                                //
                                break;
                            }
                        case DRAW_CMD_VALUE:
                            {
                                draw_value_args_t* value_args = (draw_value_args_t*)&args.value_args;
                                // TO be implemented: Custom drawing logic for value to the SRAM canvas slice
                                break;
                            }
                    }

                    if (result == INTERSECT_DONE)
                    {
                        cmd = DRAW_CMD_NOP;
                        continue;
                    }
                }

                // Blit the fast SRAM slice into our PSRAM framebuffer
                u16* psram_target = r.m_fb[r.m_current_fb] + (slice_y_start * r.m_fb_width);
                g_memcpy(psram_target, r.m_sram_canvas, r.m_sram_canvas_height * r.m_fb_width * sizeof(u16));
            }

            // The background VSYNC interrupt callback will handle the actual swap.
            r.m_fb_state = FB_READY;
        }

        enum intersect_result_t
        {
            INTERSECT_NO,    // The asset does not intersect the slice (yet) and can be skipped
            INTERSECT_YES,   // The asset intersects the slice and needs to be rendered
            INTERSECT_DONE,  // The asset when rendered is fully done and can be marked as NOP
        };

        inline intersect_result_t intersects_slice(renderer_t& r, draw_cmd_t cmd, const draw_cmd_args_t* args, i16 slice_y_start, i16 slice_y_end)
        {
            switch (cmd)
            {
                case DRAW_CMD_CLR_SCREEN:
                    {
                        return INTERSECT_YES;  // Clear screen always intersects the slice
                    }
                case DRAW_CMD_SPRITE:
                    {
                        draw_sprite_args_t* sprite_args = (draw_sprite_args_t*)&args->sprite_args;
                        // TODO
                        // Note: We assume that sprites will never 'clip' left/right of the screen, so we only need to check
                        //       the slice y start and end against the sprite y position and height.
                        break;
                    }
                case DRAW_CMD_TEXT:
                    {
                        draw_text_args_t* text_args = (draw_text_args_t*)&args->text_args;
                        // TODO
                        // Note: We assume that text will never 'clip' outside of the screen, so we only need to check
                        //       the slice y start and end against the text y position and font height.
                        break;
                    }
                case DRAW_CMD_VALUE:
                    {
                        draw_value_args_t* value_args = (draw_value_args_t*)&args->value_args;
                        // TODO
                        // Note: See note on text rendering
                        break;
                    }
            }
            return INTERSECT_NO;  // Default to no intersection
        }

        void clear_screen(u32 color)
        {
            g_renderer.m_cmd_buffer[g_renderer.m_cmd_count]                     = DRAW_CMD_CLR_SCREEN;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].clr_screen_args.color = color;
            g_renderer.m_cmd_count++;
        }

        void draw_sprite(u16 spriteId, u16 x, u16 y)
        {
            g_renderer.m_cmd_buffer[g_renderer.m_cmd_count]                        = DRAW_CMD_SPRITE;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].sprite_args.sprite_index = spriteId;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].sprite_args.x            = x;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].sprite_args.y            = y;
            g_renderer.m_cmd_count++;
        }

        void draw_text(u8 fontId, const u8* text, u16 x, u16 y, u32 color)
        {
            g_renderer.m_cmd_buffer[g_renderer.m_cmd_count]                    = DRAW_CMD_TEXT;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.font_index = fontId;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.x          = x;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.y          = y;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.color      = color;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.text       = (const char*)text;
            g_renderer.m_cmd_count++;
        }

        void draw_date(u8 fontId, u16 x, u16 y, u32 color)
        {
            g_renderer.m_cmd_buffer[g_renderer.m_cmd_count]                    = DRAW_CMD_TEXT;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.font_index = fontId;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.x          = x;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.y          = y;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.color      = color;

            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.text = g_home_state.m_data;

            g_renderer.m_cmd_count++;
        }

        void draw_time(u8 fontId, u16 x, u16 y, u32 color)
        {
            g_renderer.m_cmd_buffer[g_renderer.m_cmd_count]                    = DRAW_CMD_TEXT;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.font_index = fontId;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.x          = x;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.y          = y;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.color      = color;

            g_renderer.m_cmd_args[g_renderer.m_cmd_count].text_args.text = g_home_state.m_time;

            g_renderer.m_cmd_count++;
        }

        void draw_value(u8 fontId, i32 value, u8 unit, u16 x, u16 y, u32 color)
        {
            g_renderer.m_cmd_buffer[g_renderer.m_cmd_count]                     = DRAW_CMD_VALUE;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].value_args.font_index = fontId;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].value_args.x          = x;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].value_args.y          = y;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].value_args.color      = color;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].value_args.value      = value;
            g_renderer.m_cmd_args[g_renderer.m_cmd_count].value_args.unit       = unit;

            g_renderer.m_value_str_cursor += snprintf(g_renderer.m_value_str_buffer + g_renderer.m_value_str_cursor, g_value_str_buffer_size - g_renderer.m_value_str_cursor, "%d", value);

            g_renderer.m_cmd_count++;
        }

    }  // namespace ngui
}  // namespace ncore