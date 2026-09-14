# Current State

## Mac

- Script language compiler
- Sprite Pack Builder (also generating palettes)
- Font Pack Builder
- Asset server (for uploading assets to ESP32 device)

## ESP32

- Script language VM 
- WiFi general client framework
- LCD driver is working
- Touch driver is working
  - Gestures are working
- Rendering
  - Draw functions have scissor support

# Next Steps

## ESP32

- Script language
  - [WIP] host binding functions need to be written
- TCP client download plugin
  - [WIP] Receive Sprite Pack in PSRAM   (Max size:  4 MB)
  - [WIP] Receive Font Pack in SRAM?     (Max size: 64 kB)
  - [WIP] Receive Palette Pack in SRAM?  (Max size:  8 kB)
  - [WIP] Receive Script in SRAM?        (Max size: 64 kB)
- Rendering
  - Draw sprite with scaling support
  - 2 Command buffers for double buffered rendering in SRAM (Max size: 2*8 kB = 16 kB)
  - Build command buffer from draw calls
  - Compare command buffers and only execute when different from last frame or if
    frame buffer has been changed by other means.
  - Update frame-buffer by executing the command buffer
  - Swap command-buffers 
