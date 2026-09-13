#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "rhome/c_home.h"

#include "gui/c_draw.h"

#include "main/c_app_data.h"
#include "main/c_script_host.h"

#include "ccova/vm.h"
#include "ccova/image.h"

namespace ncore
{
    // extern(0) void ClearScreen(u32 color);
    // extern(1) void DrawSprite(u16 spriteId, u16 x, u16 y);
    // extern(1) void DrawSpriteScaled(u16 spriteId, u16 x, u16 y, u16 w, u16 h);
    // extern(2) void DrawText(u8 fontId, u8 fontSize, const u8* text, u16 x, u16 y, u32 color);
    // extern(3) void DrawDate(u8 fontId, u8 fontSize, u16 x, u16 y, u32 color);
    // extern(4) void DrawTime(u8 fontId, u8 fontSize, u16 x, u16 y, u32 color);
    // extern(5) void DrawValue(u8 fontId, u8 fontSize, i32 value, u8 unit, u16 x, u16 y, u32 color);

    enum extern_id_t
    {
        EXTERN_CLEAR_SCREEN = 0,
        EXTERN_DRAW_SPRITE,
        EXTERN_DRAW_SPRITE_SCALED,
        EXTERN_DRAW_TEXT,
        EXTERN_DRAW_DATE,
        EXTERN_DRAW_TIME,
        EXTERN_DRAW_VALUE
    };

    void extern_host_fn(void* host_context, vm_t* vm, u32 import_id)
    {
        app_data_t* app_data = (app_data_t*)host_context;

        switch (import_id)
        {
            case EXTERN_CLEAR_SCREEN:
                {
                    const u32 color = pop_bits32(vm, KindUint32);
                    ngui::clear_screen(color);
                }
                break;
            case EXTERN_DRAW_SPRITE:
                {
                    const u16 y = (u16)pop_bits32(vm, KindUint16);
                    const u16 x = (u16)pop_bits32(vm, KindUint16);
                    const u16 spriteId = (u16)pop_bits32(vm, KindUint16);
                    ngui::draw_sprite(spriteId, x, y);
                }
                break;
            case EXTERN_DRAW_SPRITE_SCALED:
                {
                    const u16 h = pop_u16(vm);
                    const u16 w = pop_u16(vm);
                    const u16 y = pop_u16(vm);
                    const u16 x = pop_u16(vm);
                    const u16 spriteId = pop_u16(vm);
                    ngui::draw_sprite_scaled(spriteId, x, y, w, h);
                }
                break;
            case EXTERN_DRAW_TEXT:
                {
                    const u32 color = pop_u32(vm);
                    const u16 y = pop_u16(vm);
                    const u16 x = pop_u16(vm);
                    const u8 fontSize = pop_u8(vm);
                    const u8 fontId = pop_u8(vm);
                    const u8* text = pop_data_pointer<u8>(vm);
                    ngui::draw_text(fontId, fontSize, text, x, y, color);
                }
                break;
            case EXTERN_DRAW_DATE:
                {
                    const u16 year = get_year(app_data->m_house);
                    const u8 month = get_month(app_data->m_house);
                    const u8 day = get_day(app_data->m_house);
                    const u8 day_of_week = get_day_of_week(app_data->m_house);
    
                    const u32 color = pop_u32(vm);
                    const u16 y = pop_u16(vm);
                    const u16 x = pop_u16(vm);
                    const u8 fontSize = pop_u8(vm);
                    const u8 fontId = pop_u8(vm);
                    ngui::draw_date(fontId, fontSize, x, y, color, year, month, day, day_of_week);
                }
                break;
            case EXTERN_DRAW_TIME:
                {
                    const u8 hour = get_hour(app_data->m_house);
                    const u8 minute = get_minute(app_data->m_house);
                    const u8 second = get_second(app_data->m_house);

                    const u32 color = pop_u32(vm);
                    const u16 y = pop_u16(vm);
                    const u16 x = pop_u16(vm);
                    const u8 fontSize = pop_u8(vm);
                    const u8 fontId = pop_u8(vm);
                    ngui::draw_time(fontId, fontSize, x, y, color, hour, minute, second);
                }
                break;
            case EXTERN_DRAW_VALUE:
                {
                    const u32 color = pop_u32(vm);
                    const u16 y = pop_u16(vm);
                    const u16 x = pop_u16(vm);
                    const u8 unit = pop_u8(vm);
                    const i32 value = pop_i32(vm);
                    const u8 fontSize = pop_u8(vm);
                    const u8 fontId = pop_u8(vm);
                    ngui::draw_value(fontId, fontSize, value, unit, x, y, color);
                }
                break;
            default:
                // Handle unknown import_id
                break;
        }
    }

}  // namespace ncore