#include "ccore/c_memory.h"
#include "ccore/c_printf.h"
#include "rcore/c_system.h"

#include "lib_guition/c_lcd.h"

#include "cgx2/c_renderer.h"
#include "cgx2/c_color.h"
#include "cgx2/c_rect.h"
#include "cgx2/c_font.h"
#include "cgx2/c_sprite.h"
#include "cgx2/c_palette.h"

#include "gui/c_draw.h"

namespace ncore
{
    namespace ngui
    {
        typedef u8  draw_cmd_t;
        typedef u16 draw_cmd_bits_t;

        const draw_cmd_t DRAW_CMD_NONE                = 0;
        const draw_cmd_t DRAW_CMD_BKGRND_COLOR        = 1;
        const draw_cmd_t DRAW_CMD_SPRITE              = 3;
        const draw_cmd_t DRAW_CMD_SPRITE_SCALED       = 4;
        const draw_cmd_t DRAW_CMD_ALPHA_SPRITE        = 5;
        const draw_cmd_t DRAW_CMD_ALPHA_SPRITE_SCALED = 6;
        const draw_cmd_t DRAW_CMD_TEXT                = 7;
        const draw_cmd_t DRAW_CMD_DATE                = 8;
        const draw_cmd_t DRAW_CMD_TIME                = 9;
        const draw_cmd_t DRAW_CMD_VALUE               = 10;
        const draw_cmd_t DRAW_CMD_DISABLED_FLAG       = 0x80;

        struct draw_bkgrnd_args_t
        {
            u16 color;  // Color to fill the screen with
        };

        struct draw_sprite_args_t
        {
            u16 x;             // X coordinate
            u16 y;             // Y coordinate
            u16 w;             // Width on screen
            u16 h;             // Height on screen
            u16 sprite_index;  // Index of the sprite in the asset database
            u8  blend_alpha;   // Alpha value for blending (0-255)
            u8  reserved;      // Reserved for future use
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

        struct draw_date_args_t
        {
            u16 x;           // X coordinate
            u16 y;           // Y coordinate
            u16 color;       // Color of the text
            u8  font_index;  // Index of the font in the asset database
            u8  size;        // Size of the text
            u16 year;
            u8  month : 4;
            u8  day_of_week : 4;
            u8  day;
        };

        struct draw_time_args_t
        {
            u16 x;           // X coordinate
            u16 y;           // Y coordinate
            u16 color;       // Color of the text
            u8  font_index;  // Index of the font in the asset database
            u8  size;        // Size of the text
            u8  hour;
            u8  minute;
            u8  second;
        };

        struct draw_value_args_t
        {
            u16 x;           // X coordinate
            u16 y;           // Y coordinate
            u16 color;       // Color of the value text
            u8  font_index;  // Index of the font in the asset database
            u8  size;        // Size of the value text
            u8  unit;        // Unit of the value (rhome, unit_type_e)
            u8  reserved0;   // Reserved for future use
            u16 reserved1;   // Reserved for future use
            i32 value;       // Integer value to be drawn
        };

        union draw_cmd_args_t
        {
            draw_bkgrnd_args_t bkgrnd_args;
            draw_sprite_args_t sprite_args;
            draw_text_args_t   text_args;
            draw_date_args_t   date_args;
            draw_time_args_t   time_args;
            draw_value_args_t  value_args;
        };

        struct cmd_buffer_t
        {
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
            char*                 m_strbuffer;  // 128
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

            // Allocate SRAM slice canvas (60 * 480 * 2 = 57600 bytes)
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
                g_renderer.m_cmd_buffer[i].m_cmd_count  = 0;
                g_renderer.m_cmd_buffer[i].m_cmd_bits   = (draw_cmd_bits_t*)nsystem::malloc(cMaxCmdCount * sizeof(draw_cmd_bits_t));
                g_renderer.m_cmd_buffer[i].m_cmd_buffer = (draw_cmd_t*)nsystem::malloc(cMaxCmdCount * sizeof(draw_cmd_t));
                g_renderer.m_cmd_buffer[i].m_cmd_args   = (draw_cmd_args_t*)nsystem::malloc(cMaxCmdCount * sizeof(draw_cmd_args_t));
            }

            g_renderer.m_strbuffer_size = 128;
            g_renderer.m_strbuffer      = (char*)nsystem::malloc(g_renderer.m_strbuffer_size);  // Buffer for multiple value strings representation

            g_renderer.m_sprite_pack  = sprite_pack;
            g_renderer.m_font_pack    = font_pack;
            g_renderer.m_palette_pack = palette_pack;
        }

        void draw_begin_frame()
        {
            renderer_t&   r         = g_renderer;
            cmd_buffer_t& curr_cmds = r.m_cmd_buffer[r.m_current_cmd_buffer];
            curr_cmds.m_cmd_count   = 0;
        }

        void draw_end_frame()
        {
            renderer_t& r = g_renderer;

            // Compare the command buffers to determine if any changes occurred.
            cmd_buffer_t& curr_cmds = r.m_cmd_buffer[r.m_current_cmd_buffer];
            cmd_buffer_t& prev_cmds = r.m_cmd_buffer[1 - r.m_current_cmd_buffer];
            if (curr_cmds.m_cmd_count == prev_cmds.m_cmd_count)
            {
                const s8 comparison = g_memcmp(curr_cmds.m_cmd_buffer, prev_cmds.m_cmd_buffer, curr_cmds.m_cmd_count * sizeof(draw_cmd_t));
                if (comparison == 0)
                {
                    // No changes detected, skip rendering this frame
                    return;
                }
            }

            // TODO
            // Pre-pass, first go over all the commands and determine which slices they intersect,
            // and mark them in the draw_cmd_t coverage bitmask.
            // Then during the actual rendering, we can skip any slices that have no coverage.
            // Each command that is executed will clear its coverage bitmask, and if it becomes zero,
            // we can skip it during the next slice rendering.

            u16*      fb        = nlcd::framebuffer();
            const u16 fb_width  = nlcd::width();
            const u16 fb_height = nlcd::height();

            ngx2::ctx_t ctx;
            ctx.m_pixels  = g_renderer.m_sram_canvas;
            ctx.m_width   = g_renderer.m_sram_canvas_width;
            ctx.m_height  = g_renderer.m_sram_canvas_height;
            ctx.m_offseth = 0;

            // For each draw command, set the slices that are covered by it in the coverage bitmask.
            {
                cmd_buffer_t& cmds = r.m_cmd_buffer[r.m_current_cmd_buffer];

                for (i16 c = 0; c < cmds.m_cmd_count; c++)
                {
                    const draw_cmd_t&      cmd  = cmds.m_cmd_buffer[c];
                    const draw_cmd_args_t& args = cmds.m_cmd_args[c];

                    switch (cmd)
                    {
                        case DRAW_CMD_SPRITE:
                        case DRAW_CMD_ALPHA_SPRITE:
                        case DRAW_CMD_SPRITE_SCALED:
                        case DRAW_CMD_ALPHA_SPRITE_SCALED:
                            {
                                draw_sprite_args_t* sprite_args = (draw_sprite_args_t*)&args.sprite_args;
                                const i16           slice_start = sprite_args->y / r.m_sram_canvas_height;
                                const i16           slice_end   = slice_start + (sprite_args->h - 1) / r.m_sram_canvas_height;
                                cmds.m_cmd_bits[c]              = (draw_cmd_bits_t)((((u32)1 << (slice_end - slice_start + 1)) - 1) << slice_start);
                                break;
                            }
                        case DRAW_CMD_TEXT:
                        case DRAW_CMD_DATE:
                        case DRAW_CMD_TIME:
                        case DRAW_CMD_VALUE:
                            {
                                draw_text_args_t*   text_args    = (draw_text_args_t*)&args.text_args;
                                const ngx2::font_t* font         = get_font(r.m_font_pack, text_args->font_index);
                                const i16           text_y_start = ngx2::get_text_y_start(font->m_ascent, text_args->y, text_args->size);
                                const i16           text_y_end   = ngx2::get_text_y_end(font->m_descent, text_args->y, text_args->size);
                                const i16           slice_start  = text_y_start / r.m_sram_canvas_height;
                                const i16           slice_end    = slice_start + (text_y_end - 1) / r.m_sram_canvas_height;
                                cmds.m_cmd_bits[c]               = (draw_cmd_bits_t)((((u32)1 << (slice_end - slice_start + 1)) - 1) << slice_start);
                                break;
                            }
                        default:
                            {
                                cmds.m_cmd_bits[c] = ~(draw_cmd_bits_t)0x0;  // Mark all slices as covering
                                break;
                            }
                    }
                }
            }

            nlcd::frame_begin();
            {
                cmd_buffer_t& cmds = r.m_cmd_buffer[r.m_current_cmd_buffer];

                const i16 num_slices = (fb_height + r.m_sram_canvas_height - 1) / r.m_sram_canvas_height;
                for (i16 i = 0; i < num_slices; i++)
                {
                    const i16 slice_y_start = i * r.m_sram_canvas_height;
                    const i16 slice_y_end   = slice_y_start + r.m_sram_canvas_height - 1;

                    ctx.m_offseth = slice_y_start;

                    for (i16 c = 0; c < cmds.m_cmd_count; c++)
                    {
                        const draw_cmd_bits_t& bits = cmds.m_cmd_bits[c];
                        if (bits & ((draw_cmd_bits_t)1 << i) == 0)
                            continue;

                        const draw_cmd_t&      cmd  = cmds.m_cmd_buffer[c];
                        const draw_cmd_args_t& args = cmds.m_cmd_args[c];

                        switch (cmd)
                        {
                            case DRAW_CMD_BKGRND_COLOR:
                                {
                                    draw_bkgrnd_args_t* bkgrnd_args = (draw_bkgrnd_args_t*)&args.bkgrnd_args;
                                    const u16           color       = bkgrnd_args->color;
                                    u16*                dst         = r.m_sram_canvas;
                                    for (i16 y = 0; y < r.m_sram_canvas_height; y++)
                                    {
                                        for (i16 x = 0; x < r.m_sram_canvas_width; x++)
                                            dst[y * r.m_sram_canvas_width + x] = color;
                                    }
                                    break;
                                }
                            case DRAW_CMD_SPRITE:
                                {
                                    draw_sprite_args_t* sprite_args = (draw_sprite_args_t*)&args.sprite_args;
                                    const ngx2::sprite_t* sprite = ngx2::get_sprite(r.m_sprite_pack, sprite_args->sprite_index);
                                    const ngx2::palette_t* palette = ngx2::get_palette(r.m_palette_pack, sprite->m_palette_index);
                                    ngx2::draw_sprite(ctx, sprite, palette, sprite_args->x, sprite_args->y, ngx2::cRgb888ColorWhite);
                                    break;
                                }
                            case DRAW_CMD_ALPHA_SPRITE:
                                {
                                    draw_sprite_args_t* sprite_args = (draw_sprite_args_t*)&args.sprite_args;
                                    const ngx2::sprite_t* sprite = ngx2::get_sprite(r.m_sprite_pack, sprite_args->sprite_index);
                                    const ngx2::palette_t* palette = ngx2::get_palette(r.m_palette_pack, sprite->m_palette_index);
                                    ngx2::draw_sprite(ctx, sprite, palette, sprite_args->x, sprite_args->y, ngx2::cRgb888ColorWhite);
                                    break;
                                }
                            case DRAW_CMD_SPRITE_SCALED:
                                {
                                    draw_sprite_args_t* sprite_args = (draw_sprite_args_t*)&args.sprite_args;
                                    const ngx2::sprite_t* sprite = ngx2::get_sprite(r.m_sprite_pack, sprite_args->sprite_index);
                                    const ngx2::palette_t* palette = ngx2::get_palette(r.m_palette_pack, sprite->m_palette_index);
                                    ngx2::draw_sprite(ctx, sprite, palette, sprite_args->x, sprite_args->y, sprite_args->w, sprite_args->h, ngx2::cRgb888ColorWhite);
                                    break;
                                }
                            case DRAW_CMD_ALPHA_SPRITE_SCALED:
                                {
                                    draw_sprite_args_t* sprite_args = (draw_sprite_args_t*)&args.sprite_args;
                                    const ngx2::sprite_t* sprite = ngx2::get_sprite(r.m_sprite_pack, sprite_args->sprite_index);
                                    const ngx2::palette_t* palette = ngx2::get_palette(r.m_palette_pack, sprite->m_palette_index);
                                    ngx2::draw_sprite(ctx, sprite, palette, sprite_args->x, sprite_args->y, sprite_args->w, sprite_args->h, ngx2::cRgb888ColorWhite);
                                    break;
                                }
                            case DRAW_CMD_TEXT:
                                {
                                    draw_text_args_t* text_args = (draw_text_args_t*)&args.text_args;
                                    const ngx2::font_t* font = ngx2::get_font(r.m_font_pack, text_args->font_index);
                                    ngx2::draw_text(ctx, font, (u16)text_args->size, text_args->x, text_args->y, (const char*)text_args->text, text_args->color);
                                    break;
                                }
                            case DRAW_CMD_DATE:
                                {
                                    draw_date_args_t* date_args = (draw_date_args_t*)&args.date_args;
                                    // TO be implemented: Custom drawing logic for date to the SRAM canvas slice
                                    break;
                                }
                            case DRAW_CMD_TIME:
                                {
                                    draw_time_args_t* time_args = (draw_time_args_t*)&args.time_args;
                                    // TO be implemented: Custom drawing logic for time to the SRAM canvas slice
                                    break;
                                }
                            case DRAW_CMD_VALUE:
                                {
                                    draw_value_args_t* value_args = (draw_value_args_t*)&args.value_args;
                                    // TO be implemented: Custom drawing logic for value to the SRAM canvas slice
                                    break;
                                }
                        }
                    }

                    // Blit the fast SRAM slice into our PSRAM framebuffer
                    u16* psram_target = fb + (slice_y_start * fb_width);
                    g_memcpy(psram_target, r.m_sram_canvas, r.m_sram_canvas_height * fb_width * sizeof(u16));
                }
            }
            nlcd::frame_end();  // note: blocking call, vsync wait occurs here

            // Switch to the other command buffer for the next frame
            r.m_current_cmd_buffer = 1 - r.m_current_cmd_buffer;
        }

        void clear_background(u32 color)
        {
            cmd_buffer_t& cmds                                  = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];
            cmds.m_cmd_buffer[cmds.m_cmd_count]                 = DRAW_CMD_BKGRND_COLOR;
            cmds.m_cmd_bits[cmds.m_cmd_count]                   = 0;
            cmds.m_cmd_args[cmds.m_cmd_count].bkgrnd_args.color = color;
            cmds.m_cmd_count++;
        }

        void draw_background(u16 spriteId, u8 fade)
        {
            ngx2::sprite_t* sprite = ngx2::get_sprite(g_renderer.m_sprite_pack, (u32)spriteId);
            if (!sprite)
                return;

            cmd_buffer_t& cmds = g_renderer.m_cmd_buffer[g_renderer.m_current_cmd_buffer];
            if (sprite->m_width == g_renderer.m_fb_width && sprite->m_height == g_renderer.m_fb_height)
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

        void draw_sprite_using_alpha(u16 spriteId, u16 x, u16 y, u8 alpha)
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
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.w            = sprite->m_width;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.h            = sprite->m_height;
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.blend_alpha  = alpha;

            cmds.m_cmd_count++;
        }

        void draw_sprite_using_alpha(u16 spriteId, u16 x, u16 y, u16 w, u16 h, u8 alpha)
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
            cmds.m_cmd_args[cmds.m_cmd_count].sprite_args.blend_alpha  = alpha;
            cmds.m_cmd_count++;
        }

        void draw_sprite(u16 spriteId, u16 x, u16 y) { draw_sprite_using_alpha(spriteId, x, y, 0, 0, 255); }
        void draw_sprite(u16 spriteId, u16 x, u16 y, u16 w, u16 h) { draw_sprite_using_alpha(spriteId, x, y, w, h, 255); }

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

            cmds.m_cmd_buffer[cmds.m_cmd_count]                    = DRAW_CMD_DATE;
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

            cmds.m_cmd_buffer[cmds.m_cmd_count]                     = DRAW_CMD_TIME;
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