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
            MessageTypeClientInfo  = 0xC001,
            MessageTypeFrameUpdate = 0xC002,
            MessageTypeInputEvent  = 0xC003
        };

        // - Frame update message: [MessageTypeFrameUpdate(u16), MessageLen(u16), flags(u16)]
        static inline u8* write_frame_update_msg(u8* buffer, u16 flags)
        {
            const u16 MsgLen = 6;  
            buffer[0] = MessageTypeFrameUpdate;
            buffer[1] = MessageTypeFrameUpdate >> 8;
            buffer[2] = MsgLen & 0xFF;
            buffer[3] = (MsgLen >> 8) & 0xFF;
            buffer[4] = flags & 0xFF;
            buffer[5] = (flags >> 8) & 0xFF;
            return buffer + MsgLen;
        }

        enum InputType : u16
        {
            InputTypeTouch  = 0x1201,
            InputTypeSwipe  = 0x1202,
            InputTypeButton = 0x1203,
            InputTypeRotary = 0x1204
        };

        // - Touch event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeTouch(u16), x(int16), y(int16)]
        static inline u8* write_touch_event(u8* buffer, s16 x, s16 y)
        {
            const u16 MsgLen = 10;
            buffer[0]        = MessageTypeInputEvent;
            buffer[1]        = MessageTypeInputEvent >> 8;
            buffer[2]        = MsgLen & 0xFF;
            buffer[3]        = (MsgLen >> 8) & 0xFF;
            buffer[4]        = InputTypeTouch;
            buffer[5]        = InputTypeTouch >> 8;
            buffer[6]        = (u8)(x & 0xFF);
            buffer[7]        = (u8)((x >> 8) & 0xFF);
            buffer[8]        = (u8)(y & 0xFF);
            buffer[9]        = (u8)((y >> 8) & 0xFF);
            return buffer + MsgLen;
        }

        // - Swipe event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeSwipe(u16), direction(int8), distance(int16)]
        static inline u8* write_swipe_event(u8* buffer, s8 direction, s16 distance)
        {
            const u16 MsgLen = 10;
            buffer[0]        = MessageTypeInputEvent;
            buffer[1]        = MessageTypeInputEvent >> 8;
            buffer[2]        = MsgLen & 0xFF;
            buffer[3]        = (MsgLen >> 8) & 0xFF;
            buffer[4]        = InputTypeSwipe;
            buffer[5]        = InputTypeSwipe >> 8;
            buffer[6]        = (u8)direction;
            buffer[7]        = 0;  // padding for alignment
            buffer[8]        = (u8)(distance & 0xFF);
            buffer[9]        = (u8)((distance >> 8) & 0xFF);
            return buffer + MsgLen;
        }

        // - Button event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeButton(u16), buttonId(int16), state(int16)]
        static inline u8* write_button_event(u8* buffer, s16 buttonId, s16 state)
        {
            const u16 MsgLen = 10;
            buffer[0]        = MessageTypeInputEvent;
            buffer[1]        = MessageTypeInputEvent >> 8;
            buffer[2]        = MsgLen & 0xFF;
            buffer[3]        = (MsgLen >> 8) & 0xFF;
            buffer[4]        = InputTypeButton;
            buffer[5]        = InputTypeButton >> 8;
            buffer[6]        = (u8)(buttonId & 0xFF);
            buffer[7]        = (u8)((buttonId >> 8) & 0xFF);
            buffer[8]        = (u8)(state & 0xFF);
            buffer[9]        = (u8)((state >> 8) & 0xFF);
            return buffer + MsgLen;
        }

        // - Rotary event: [MessageTypeInputEvent(u16), MessageLen(u16), InputTypeRotary(u16), rotation(int32)]
        static inline u8* write_rotary_event(u8* buffer, s32 rotation)
        {
            const u16 MsgLen = 10;
            buffer[0]        = MessageTypeInputEvent;
            buffer[1]        = MessageTypeInputEvent >> 8;
            buffer[2]        = MsgLen & 0xFF;
            buffer[3]        = (MsgLen >> 8) & 0xFF;
            buffer[4]        = InputTypeRotary;
            buffer[5]        = InputTypeRotary >> 8;
            buffer[6]        = (u8)(rotation & 0xFF);
            buffer[7]        = (u8)((rotation >> 8) & 0xFF);
            buffer[8]        = (u8)((rotation >> 16) & 0xFF);
            buffer[9]        = (u8)((rotation >> 24) & 0xFF);
            return buffer + MsgLen;
        }

    }  // namespace nmui
}  // namespace ncore

#endif  // __RMUI_INPUT_MSGS_H__