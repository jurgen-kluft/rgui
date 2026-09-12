#include "rcore/c_app.h"
#include "rcore/c_log.h"

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
        app_data_t* app = (app_data_t*)host_context;

        switch (import_id)
        {
            case EXTERN_CLEAR_SCREEN:
                // Handle ClearScreen
                break;
            case EXTERN_DRAW_SPRITE:
                // Handle DrawSprite
                break;
            case EXTERN_DRAW_SPRITE_SCALED:
                // Handle DrawSpriteScaled
                break;
            case EXTERN_DRAW_TEXT:
                // Handle DrawText
                break;
            case EXTERN_DRAW_DATE:
                // Handle DrawDate
                break;
            case EXTERN_DRAW_TIME:
                // Handle DrawTime
                break;
            case EXTERN_DRAW_VALUE:
                // Handle DrawValue
                break;
            default:
                // Handle unknown import_id
                break;
        }



    }


}  // namespace ncore