#ifndef __RMUI_INPUT_MSGS_H__
#define __RMUI_INPUT_MSGS_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nmui
    {
        enum MsgType : u16
        {
            MessageTypeInputEvent = 0xC003
        };

        enum InputType : u16
        {
            InputTypeSingleTap = 0x1201,
            InputTypeDoubleTap = 0x1202,
            InputTypeSwipe     = 0x1203,
            InputTypeButton    = 0x1204,
            InputTypeRotary    = 0x1205
        };
        static inline u8* write_u16(u8* buffer, u16 value)
        {
            buffer[0] = (u8)(value & 0xFF);
            buffer[1] = (u8)((value >> 8) & 0xFF);
            return buffer + 2;
        }
        static inline u8* write_u32(u8* buffer, u32 value)
        {
            buffer[0] = (u8)(value & 0xFF);
            buffer[1] = (u8)((value >> 8) & 0xFF);
            buffer[2] = (u8)((value >> 16) & 0xFF);
            buffer[3] = (u8)((value >> 24) & 0xFF);
            return buffer + 4;
        }

        // - Tap event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeSingleTap(u16), x(int16), y(int16)]
        static inline u16 write_single_tap_event(u8* buffer, s16 x, s16 y)
        {
            const u16 MsgLen = 10;
            u8*       p      = buffer;
            p                = write_u16(p, MessageTypeInputEvent);
            p                = write_u16(p, MsgLen);
            p                = write_u16(p, InputTypeSingleTap);
            p                = write_u16(p, (u16)x);
            p                = write_u16(p, (u16)y);
            ASSERTS((u16)(p - buffer) == MsgLen, "Unexpected message length for touch event");
            return MsgLen;
        }

        // - Double tap event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeDoubleTap(u16), x(int16), y(int16)]
        static inline u16 write_double_tap_event(u8* buffer, s16 x, s16 y)
        {
            const u16 MsgLen = 10;
            u8*       p      = buffer;
            p                = write_u16(p, MessageTypeInputEvent);
            p                = write_u16(p, MsgLen);
            p                = write_u16(p, InputTypeDoubleTap);
            p                = write_u16(p, (u16)x);
            p                = write_u16(p, (u16)y);
            ASSERTS((u16)(p - buffer) == MsgLen, "Unexpected message length for touch event");
            return MsgLen;
        }

        // - Swipe event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeSwipe(u16), fingers(int16), direction(int16), distance(int16)]
        static inline u16 write_swipe_event(u8* buffer, s8 fingers, s8 direction, s16 distance)
        {
            const u16 MsgLen = 12;
            u8*       p      = buffer;
            p                = write_u16(p, MessageTypeInputEvent);
            p                = write_u16(p, MsgLen);
            p                = write_u16(p, InputTypeSwipe);
            p                = write_u16(p, (u16)fingers);
            p                = write_u16(p, (u16)direction);
            p                = write_u16(p, (u16)distance);
            ASSERTS((u16)(p - buffer) == MsgLen, "Unexpected message length for swipe event");
            return MsgLen;
        }

        // - Button event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeButton(u16), buttonId(int16), state(int16)]
        static inline u16 write_button_event(u8* buffer, s16 buttonId, s16 state)
        {
            const u16 MsgLen = 10;
            u8*       p      = buffer;
            p                = write_u16(p, MessageTypeInputEvent);
            p                = write_u16(p, MsgLen);
            p                = write_u16(p, InputTypeButton);
            p                = write_u16(p, (u16)buttonId);
            p                = write_u16(p, (u16)state);
            ASSERTS((u16)(p - buffer) == MsgLen, "Unexpected message length for button event");
            return MsgLen;
        }

        // - Rotary event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeRotary(u16), rotation(int32)]
        static inline u16 write_rotary_event(u8* buffer, s32 rotation)
        {
            const u16 MsgLen = 10;
            u8*       p      = buffer;
            p                = write_u16(p, MessageTypeInputEvent);
            p                = write_u16(p, MsgLen);
            p                = write_u16(p, InputTypeRotary);
            p                = write_u32(p, (u32)rotation);
            ASSERTS((u16)(p - buffer) == MsgLen, "Unexpected message length for rotary event");
            return MsgLen;
        }

    }  // namespace nmui
}  // namespace ncore

#endif  // __RMUI_INPUT_MSGS_H__