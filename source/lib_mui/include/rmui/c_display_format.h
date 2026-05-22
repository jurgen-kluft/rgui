#ifndef __RMUI_DISPLAY_FORMAT_H__
#define __RMUI_DISPLAY_FORMAT_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nmui
    {
        // BPP Configuration ID Sub-Field (Bits [15:12])
        enum display_bpp_t
        {
            BPP_1  = 0x0,  // 1 bit per pixel
            BPP_2  = 0x1,  // 2 bits per pixel (e.g., multi-color E-Paper)
            BPP_4  = 0x2,  // 4 bits per pixel
            BPP_8  = 0x3,  // 8 bits per pixel
            BPP_16 = 0x4,  // 16 bits per pixel
            BPP_24 = 0x5,  // 24 bits per pixel
            BPP_32 = 0x6   // 32 bits per pixel
            // 0x7 to 0xF are reserved for future definitions (16 total slots available)
        };

        // Endianness Sub-Field (Bits [11:10])
        enum display_endian_t
        {
            ENDIAN_LITTLE      = 0x0,  // Standard Little-Endian
            ENDIAN_BIG_SWAPPED = 0x1,  // Big-Endian byte order (SPI DMA optimized)
            ENDIAN_RESERVED_A  = 0x2,  // Available permutation slot
            ENDIAN_RESERVED_B  = 0x3   // Available permutation slot
        };

        // Layout Configuration Sub-Field (Bits [9:6])
        enum display_layout_t
        {
            LAYOUT_LINEAR       = 0x0,  // Standard continuous left-to-right rows
            LAYOUT_MONO_V_PAGE  = 0x1,  // Page orientation (SSD1306 vertical chunks)
            LAYOUT_MONO_H_SHARP = 0x2,  // Sharp framing layout with line headers/trailers
            LAYOUT_TILED        = 0x3   // Micro-tile blocked graphics memory
            // 0x4 to 0xF are reserved for future layouts (16 total slots available)
        };

        // Channel Composition Profile Sub-Field (Bits [5:0])
        enum display_profile_t
        {
            PROFILE_MONO      = 0x01,  // 1-bit monochrome channel rules
            PROFILE_GRAY_4BIT = 0x02,  // 4-bit grayscale packing
            PROFILE_RGB565    = 0x03,  // Color channel distribution 5-6-5 (RGB)
            PROFILE_BGR565    = 0x04,  // Color channel distribution 5-6-5 (BGR)
            PROFILE_RGB888    = 0x05,  // 24-bit color packing sequence (RGB)
            PROFILE_BGR888    = 0x06,  // 24-bit color packing sequence (BGR)
            PROFILE_ARGB8888  = 0x07   // 32-bit alpha composition pipeline (ARGB)
            // 0x08 to 0x3F are reserved for future target variations (64 total slots available)
        };

// Combines sub-fields: BPP ID [15:12], Endian [11:10], Layout [9:6], Color Profile [5:0]
#define D_ENCODE_DISPLAY_FORMAT_16(bpp_id, endian, layout, profile) ((u16)((((bpp_id) & 0x0F) << 12) | (((endian) & 0x03) << 10) | (((layout) & 0x0F) << 6) | ((profile) & 0x3F)))

        enum display_format_t
        {
            // --- Monochrome / Grayscale Layouts ---
            DF_MONO_V_PAGE   = D_ENCODE_DISPLAY_FORMAT_16(BPP_1, ENDIAN_LITTLE, LAYOUT_MONO_V_PAGE, PROFILE_MONO),
            DF_MONO_H_LINEAR = D_ENCODE_DISPLAY_FORMAT_16(BPP_1, ENDIAN_LITTLE, LAYOUT_LINEAR, PROFILE_MONO),
            DF_MONO_H_SHARP  = D_ENCODE_DISPLAY_FORMAT_16(BPP_1, ENDIAN_LITTLE, LAYOUT_MONO_H_SHARP, PROFILE_MONO),
            DF_GRAY_4BIT     = D_ENCODE_DISPLAY_FORMAT_16(BPP_4, ENDIAN_LITTLE, LAYOUT_LINEAR, PROFILE_GRAY_4BIT),

            // --- 16-Bit Color Panels ---
            DF_RGB565_GENERIC      = D_ENCODE_DISPLAY_FORMAT_16(BPP_16, ENDIAN_LITTLE, LAYOUT_LINEAR, PROFILE_RGB565),
            DF_RGB565_BYTE_SWAPPED = D_ENCODE_DISPLAY_FORMAT_16(BPP_16, ENDIAN_BIG_SWAPPED, LAYOUT_LINEAR, PROFILE_RGB565),
            DF_BGR565_GENERIC      = D_ENCODE_DISPLAY_FORMAT_16(BPP_16, ENDIAN_LITTLE, LAYOUT_LINEAR, PROFILE_BGR565),

            // --- High-Resolution True Color / Alpha Channels ---
            DF_RGB888   = D_ENCODE_DISPLAY_FORMAT_16(BPP_24, ENDIAN_LITTLE, LAYOUT_LINEAR, PROFILE_RGB888),
            DF_BGR888   = D_ENCODE_DISPLAY_FORMAT_16(BPP_24, ENDIAN_LITTLE, LAYOUT_LINEAR, PROFILE_BGR888),
            DF_ARGB8888 = D_ENCODE_DISPLAY_FORMAT_16(BPP_32, ENDIAN_LITTLE, LAYOUT_LINEAR, PROFILE_ARGB8888)
        };

        static inline display_bpp_t     display_format_get_bpp_id(u16 format) { return static_cast<display_bpp_t>((format >> 12) & 0x0F); }
        static inline display_endian_t  display_format_get_endian(u16 format) { return static_cast<display_endian_t>((format >> 10) & 0x03); }
        static inline display_layout_t  display_format_get_layout(u16 format) { return static_cast<display_layout_t>((format >> 6) & 0x0F); }
        static inline display_profile_t display_format_get_profile(u16 format) { return static_cast<display_profile_t>(format & 0x3F); }

        static inline u8 display_bpp_to_literal_bits(display_bpp_t bpp_id)
        {
            // 16 slots corresponding to the 4-bit field depth allocation [15:12]
            static const u8 bpp_lookup_table[16] = {
              1,   // BPP_1  (0x0) -> 1 bit per pixel
              2,   // BPP_2  (0x1) -> 2 bits per pixel
              4,   // BPP_4  (0x2) -> 4 bits per pixel
              8,   // BPP_8  (0x3) -> 8 bits per pixel
              16,  // BPP_16 (0x4) -> 16 bits per pixel
              24,  // BPP_24 (0x5) -> 24 bits per pixel
              32,  // BPP_32 (0x6) -> 32 bits per pixel
              0,   // 0x7 -> Reserved / Unused
              0,   // 0x8 -> Reserved / Unused
              0,   // 0x9 -> Reserved / Unused
              0,   // 0xA -> Reserved / Unused
              0,   // 0xB -> Reserved / Unused
              0,   // 0xC -> Reserved / Unused
              0,   // 0xD -> Reserved / Unused
              0,   // 0xE -> Reserved / Unused
              0    // 0xF -> Reserved / Unused
            };

            // Cast to index safely; the bitfield mask guarantees the value stays under 16
            u8 index = static_cast<u8>(bpp_id);
            return (index < 16) ? bpp_lookup_table[index] : 0;
        }
    }  // namespace nmui
}  // namespace ncore

#endif  // __RMUI_DISPLAY_FORMAT_H__