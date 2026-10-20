#include "ccore/c_memory.h"
#include "ccore/c_printf.h"
#include "rcore/c_system.h"

#include "lib_guition/c_lcd.h"

#include "cgx2/c_slice_based_renderer.h"

#include "gui/c_draw.h"

namespace ncore
{
    namespace ngui
    {
        typedef u8  draw_cmd_t;
        typedef u16 draw_cmd_bits_t;

        const draw_cmd_t DRAW_CMD_NOP           = 0;
        const draw_cmd_t DRAW_CMD_BKGRND_COLOR  = 1;
        const draw_cmd_t DRAW_CMD_SPRITE        = 3;
        const draw_cmd_t DRAW_CMD_SPRITE_SCALED = 4;
        const draw_cmd_t DRAW_CMD_TEXT          = 5;
        const draw_cmd_t DRAW_CMD_VALUE         = 6;

        struct draw_bkgrnd_args_t
        {
            u16 color;  // Color to fill the screen with
        };

        struct draw_sprite_args_t
        {
            u16 x;              // X coordinate
            u16 y;              // Y coordinate
            u16 w;              // Width on screen
            u16 h;              // Height on screen
            u16 sprite_index;   // Index of the sprite in the asset database
            u16 palette_index;  // Index of the palette in the asset database
            u8  blend_alpha;    // Alpha value for blending (0-255)
            u8  reserved;       // Reserved for future use
        };

        struct draw_text_args_t
        {
            u16 x;           // X coordinate
            u16 y;           // Y coordinate
            u16 color;       // Color of the text
            u8  font_index;  // Index of the font in the asset database
            u8  size;        // Size of the text
            union
            {
                const u8* text;  // Pointer to the text string to be drawn
                struct
                {
                    u16 year;
                    u8  month : 4;
                    u8  day_of_week : 4;
                    u8  day;
                } date;
                struct
                {
                    u8 hour;
                    u8 minute;
                    u8 second;
                } time;
            };
        };

        struct draw_value_args_t
        {
            u16 x;           // X coordinate
            u16 y;           // Y coordinate
            u16 color;       // Color of the value text
            u8  font_index;  // Index of the font in the asset database
            u8  size;        // Size of the value text
            u8  unit;        // Unit of the value (e.g., 0 for none, 1 for px, etc.)
            u8  reserved0;   // Reserved for future use
            u16 reserved1;   // Reserved for future use
            i32 value;       // Integer value to be drawn
        };

        union draw_cmd_args_t
        {
            draw_bkgrnd_args_t bkgrnd_args;
            draw_sprite_args_t sprite_args;
            draw_text_args_t   text_args;
            draw_value_args_t  value_args;
        };

        enum cmd_buffer_state_t
        {
            CMD_BUFFER_IDLE,       // Buffer is ready to be written to
            CMD_BUFFER_WAITING,    // Buffer is filled but not yet rendered
            CMD_BUFFER_RENDERED,   // Buffer is fully rendered into the framebuffer
            CMD_BUFFER_PRESENTED,  // Buffer has been presented on the screen
        };

        struct cmd_buffer_t
        {
            volatile u16     m_state;
            u16              m_cmd_count;
            draw_cmd_t*      m_cmd_buffer;
            draw_cmd_bits_t* m_cmd_bits;
            draw_cmd_args_t* m_cmd_args;
        };

        struct renderer_t
        {
            u16                   m_fb_width;
            u16                   m_fb_height;
            u16*                  m_sram_canvas;
            u16                   m_sram_canvas_height;
            u16                   m_sram_canvas_width;
            u16                   m_current_cmd_buffer;
            i16                   m_strbuffer_size;
            char*                 m_strbuffer;  // 512
            cmd_buffer_t          m_cmd_buffer[2];
            ngx2::sprite_pack_t*  m_sprite_pack;
            ngx2::font_pack_t*    m_font_pack;
            ngx2::palette_pack_t* m_palette_pack;
        };

        const static i32 cMaxCmdCount = 128;  // Maximum number of draw commands per frame

        static renderer_t g_renderer;

        void init_renderer(u16 sram_canvas_height, ngx2::sprite_pack_t* sprite_pack, ngx2::font_pack_t* font_pack, ngx2::palette_pack_t* palette_pack)
        {
            g_renderer.m_current_cmd_buffer = 0;

            g_renderer.m_fb_height = nlcd::height();
            g_renderer.m_fb_width  = nlcd::width();

            // Allocate SRAM canvas (60 * 480 * 2 = 57600 bytes)
            g_renderer.m_sram_canvas_height = sram_canvas_height;
            g_renderer.m_sram_canvas_width  = g_renderer.m_fb_width;
            g_renderer.m_sram_canvas        = (u16*)nsystem::malloc(sram_canvas_height * g_renderer.m_fb_width * sizeof(u16));

            // Allocate command buffers and arguments
            //    cMaxCmdCount * 1  = 128 bytes
            //    cMaxCmdCount * 2  = 256 bytes
            //    cMaxCmdCount * 16 = 2048 bytes
            // Total: 2 * (128 + 256 + 2048) = 4864 bytes
            for (i32 i = 0; i < 2; ++i)
            {
                g_renderer.m_cmd_buffer[i].m_state      = CMD_BUFFER_IDLE;
                g_renderer.m_cmd_buffer[i].m_cmd_count  = 0;
                g_renderer.m_cmd_buffer[i].m_cmd_bits   = (draw_cmd_bits_t*)nsystem::malloc(cMaxCmdCount * sizeof(draw_cmd_bits_t));
                g_renderer.m_cmd_buffer[i].m_cmd_buffer = (draw_cmd_t*)nsystem::malloc(cMaxCmdCount * sizeof(draw_cmd_t));
                g_renderer.m_cmd_buffer[i].m_cmd_args   = (draw_cmd_args_t*)nsystem::malloc(cMaxCmdCount * sizeof(draw_cmd_args_t));
            }

            g_renderer.m_strbuffer_size = 512;
            g_renderer.m_strbuffer      = (char*)nsystem::malloc(g_renderer.m_strbuffer_size);  // Buffer for multiple value strings representation

            g_renderer.m_sprite_pack  = sprite_pack;
            g_renderer.m_font_pack    = font_pack;
            g_renderer.m_palette_pack = palette_pack;
        }

        void draw_begin_frame()
        {
            renderer_t& r = g_renderer;

            if (r.m_cmd_buffer[r.m_current_cmd_buffer].m_state == CMD_BUFFER_IDLE)
            {
                r.m_cmd_buffer[r.m_current_cmd_buffer].m_cmd_count = 0;
                return;
            }
        }

        void draw_end_frame()
        {
            renderer_t& r = g_renderer;

            // Skip if the previous frame hasn't been picked up by the VSYNC interrupt yet
            if (r.m_cmd_buffer[1 - r.m_current_cmd_buffer].m_state != CMD_BUFFER_IDLE)
            {
                return;
            }

            // TODO
            // Pre-pass, first go over all the commands and determine which slices they intersect,
            // and mark them in the draw_cmd_t coverage bitmask.
            // Then during the actual rendering, we can skip any slices that have no coverage.
            // Each command that is executed will clear its coverage bitmask, and if it becomes zero,
            // we can skip it during the next slice rendering.

            u16* fb        = nullptr;
            u16  fb_width  = 0;
            u16  fb_height = 0;
            nlcd::draw_begin_frame(fb, fb_width, fb_height);
            {
                cmd_buffer_t& cmds = r.m_cmd_buffer[r.m_current_cmd_buffer];

                const i16 num_slices = (fb_height + r.m_sram_canvas_height - 1) / r.m_sram_canvas_height;
                for (i16 i = 0; i < num_slices; i++)
                {
                    const i16 slice_y_start = i * r.m_sram_canvas_height;
                    const i16 slice_y_end   = slice_y_start + r.m_sram_canvas_height - 1;

                    g_memclr(r.m_sram_canvas, r.m_sram_canvas_height * r.m_sram_canvas_width * sizeof(u16));
                    for (i16 c = 0; c < cmds.m_cmd_count; c++)
                    {
                        draw_cmd_t& cmd = cmds.m_cmd_buffer[c];
                        if (cmd == DRAW_CMD_NOP)
                            continue;

                        const draw_cmd_args_t& args   = cmds.m_cmd_args[c];
                        intersect_result_t     result = intersects_slice(r, cmd, &args, slice_y_start, slice_y_end);
                        if (result == INTERSECT_NO)
                            continue;

                        switch (cmd)
                        {
                            case DRAW_CMD_BKGRND_COLOR:
                                {
                                    draw_bkgrnd_args_t* bkgrnd_args = (draw_bkgrnd_args_t*)&args.bkgrnd_args;
                                    g_memset(r.m_sram_canvas, bkgrnd_args->color, r.m_sram_canvas_height * r.m_sram_canvas_width * sizeof(u16));
                                    break;
                                }
                            case DRAW_CMD_SPRITE:
                                {
                                    draw_sprite_args_t* sprite_args = (draw_sprite_args_t*)&args.sprite_args;
                                    //
                                    break;
                                }
                            case DRAW_CMD_SPRITE_SCALED:
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
                    u16* psram_target = fb + (slice_y_start * fb_width);
                    g_memcpy(psram_target, r.m_sram_canvas, r.m_sram_canvas_height * fb_width * sizeof(u16));
                }

                nlcd::draw_end_frame();
            }
            // Mark the current command buffer as waiting to be presented
            r.m_cmd_buffer[r.m_current_cmd_buffer].m_state = CMD_BUFFER_WAITING;

            // Switch to the other command buffer for the next frame
            r.m_current_cmd_buffer = 1 - r.m_current_cmd_buffer;
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
                case DRAW_CMD_BKGRND_COLOR:
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
                case DRAW_CMD_SPRITE_SCALED:
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

        void clear_background(u32 color)
        {
            cmd_buffer_t& cmds                                  = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];
            cmds.m_cmd_buffer[cmds.m_cmd_count]                 = DRAW_CMD_BKGRND_COLOR;
            cmds.m_cmd_bits[cmds.m_cmd_count]                   = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].bkgrnd_args.color = color;
            cmds.m_cmd_count++;
        }

        void draw_background(u16 spriteId)
        {
            ngx2::sprite_t* sprite = ngx2::get_sprite(g_renderer.m_sprite_pack, (u32)spriteId);
            if (!sprite)
                return;

            cmd_buffer_t& cmds = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];
            if (sprite->width == g_renderer.m_fb_width && sprite->height == g_renderer.m_fb_height)
            {
                cmds.m_cmd_buffer[cmds.m_cmd_count]                        = DRAW_CMD_SPRITE;
                cmds.m_cmd_bits[cmds.m_cmd_count]                          = 0;
                cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.sprite_index = spriteId;
                cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.x            = 0;
                cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.y            = 0;
                cmds.m_cmd_count++;
            }
            else
            {
                cmds.m_cmd_buffer[cmds.m_cmd_count]                        = DRAW_CMD_SPRITE_SCALED;
                cmds.m_cmd_bits[cmds.m_cmd_count]                          = 0;
                cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.sprite_index = spriteId;
                cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.x            = 0;
                cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.y            = 0;
                cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.w            = g_renderer.m_fb_width;
                cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.h            = g_renderer.m_fb_height;
                cmds.m_cmd_count++;
            }
        }

        void draw_sprite(u16 spriteId, u16 x, u16 y)
        {
            ngx2::sprite_t* sprite = ngx2::get_sprite(g_renderer.m_sprite_pack, (u32)spriteId);
            if (!sprite)
                return;

            cmd_buffer_t& cmds = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];

            cmds.m_cmd_buffer[cmds.m_cmd_count]                        = DRAW_CMD_SPRITE;
            cmds.m_cmd_bits[cmds.m_cmd_count]                          = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.sprite_index = spriteId;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.x            = x;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.y            = y;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.w            = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.h            = 0;

            cmds.m_cmd_count++;
        }

        void draw_sprite(u16 spriteId, u16 x, u16 y, u16 w, u16 h)
        {
            ngx2::sprite_t* sprite = ngx2::get_sprite(g_renderer.m_sprite_pack, (u32)spriteId);
            if (!sprite)
            {
                // We should maybe use a default sprite here to easily spot missing assets in the renderer
                return;
            }

            cmd_buffer_t& cmds = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];

            cmds.m_cmd_buffer[cmds.m_cmd_count]                        = DRAW_CMD_SPRITE_SCALED;
            cmds.m_cmd_bits[cmds.m_cmd_count]                          = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.sprite_index = spriteId;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.x            = x;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.y            = y;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.w            = w;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.h            = h;
            cmds.m_cmd_count++;
        }

        void draw_text(u8 fontId, u16 size, const u8* text, u16 x, u16 y, u32 color)
        {
            ngx2::font_t* font = ngx2::get_font(g_renderer.m_font_pack, (u32)fontId);
            if (!font)
                return;

            cmd_buffer_t& cmds = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];

            cmds.m_cmd_buffer[cmds.m_cmd_count]                    = DRAW_CMD_TEXT;
            cmds.m_cmd_bits[cmds.m_cmd_count]                      = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.font_index = fontId;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.size       = size;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.x          = x;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.y          = y;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.color      = color;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.text       = text;
            cmds.m_cmd_count++;
        }

        void draw_date(u8 fontId, u16 size, u16 x, u16 y, u32 color, u16 year, u8 month, u8 day, u8 day_of_week)
        {
            ngx2::font_t* font = ngx2::get_font(g_renderer.m_font_pack, (u32)fontId);
            if (!font)
                return;

            cmd_buffer_t& cmds = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];

            cmds.m_cmd_buffer[cmds.m_cmd_count]                    = DRAW_CMD_TEXT;
            cmds.m_cmd_bits[cmds.m_cmd_count]                      = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.font_index = fontId;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.size       = size;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.x          = x;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.y          = y;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.color      = color;

            cmds.m_cmd_args[cmds.m_cmd_count].text_args.date.year        = year;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.date.month       = month;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.date.day         = day;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.date.day_of_week = day_of_week;

            cmds.m_cmd_count++;
        }

        void draw_time(u8 fontId, u16 size, u16 x, u16 y, u32 color, u8 hour, u8 minute, u8 second)
        {
            ngx2::font_t* font = ngx2::get_font(g_renderer.m_font_pack, (u32)fontId);
            if (!font)
                return;

            cmd_buffer_t& cmds = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];

            cmds.m_cmd_buffer[cmds.m_cmd_count]                     = DRAW_CMD_TEXT;
            cmds.m_cmd_bits[cmds.m_cmd_count]                       = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.font_index  = fontId;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.size        = size;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.x           = x;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.y           = y;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.color       = color;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.time.hour   = hour;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.time.minute = minute;
            cmds.m_cmd_args[cmds.m_cmd_count].text_args.time.second = second;

            cmds.m_cmd_count++;
        }

        void draw_value(u8 fontId, u16 size, i32 value, u8 unit, u16 x, u16 y, u32 color)
        {
            ngx2::font_t* font = ngx2::get_font(g_renderer.m_font_pack, (u32)fontId);
            if (!font)
                return;

            cmd_buffer_t& cmds = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];

            cmds.m_cmd_buffer[cmds.m_cmd_count]                     = DRAW_CMD_VALUE;
            cmds.m_cmd_bits[cmds.m_cmd_count]                       = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].value_args.font_index = fontId;
            cmds.m_cmd_args[cmds.m_cmd_count].value_args.size       = size;
            cmds.m_cmd_args[cmds.m_cmd_count].value_args.x          = x;
            cmds.m_cmd_args[cmds.m_cmd_count].value_args.y          = y;
            cmds.m_cmd_args[cmds.m_cmd_count].value_args.color      = color;
            cmds.m_cmd_args[cmds.m_cmd_count].value_args.value      = value;
            cmds.m_cmd_args[cmds.m_cmd_count].value_args.unit       = unit;

            cmds.m_cmd_count++;
        }

    }  // namespace ngui
}  // namespace ncore