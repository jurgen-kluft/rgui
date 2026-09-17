# Current State

## Mac

- Script language compiler
- Sprite Pack Builder (also generating palettes)
- Font Pack Builder
- Asset server (for uploading assets to ESP32 device)

## ESP32

- WiFi general client framework
- Script language VM 
- LCD driver is working
- Touch driver is working
  - Gestures are working
- Rendering
  - Slice based 2D renderer

# Next Steps

## ESP32

- Script language
  - [WIP] host binding functions need to be written
- TCP client download plugin (can download directly in PSRAM)
  - [WIP] Receive Sprite Pack in PSRAM    (Max size:  4 MB)
  - [WIP] Receive Font Pack in SRAM?      (Max size: 64 kB)
  - [WIP] Receive Palette Pack in SRAM?   (Max size:  8 kB, ~16 palettes)
  - [WIP] Receive Script in SRAM?         (Max size: 64 kB, or 32 kB?)
  - [WIP] Receive House Meta in SRAM?     (Max size:  2 kB)
  - [WIP] Receive House Data in SRAM?     (Max size:  2 kB)
- Rendering
  - Draw sprite with scaling support
  - Screen Slice                                            (Max size: 60 kB (60 x 480 x 2))
  - 2 Command buffers for double buffered rendering in SRAM (Max size:  8 kB (2 * 4 kB))
  - Build command buffer from draw calls
  - Compare command buffers and only execute when different from last frame or if
    frame buffer has been changed by other means.
  - Update frame-buffer by executing the command buffer
  - Swap command-buffers 
