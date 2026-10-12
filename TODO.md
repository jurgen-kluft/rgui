# Current State

## Mac

- [x] Script language compiler / linker / VM
- [x] Sprite Pack Builder (also generating palettes)
- [x] Font Pack Builder
- [x] Asset server (for uploading assets to ESP32 device)

## ESP32

- [x] WiFi general client framework
- [x] Script language VM 
- [x] LCD driver is working
- [x] Touch driver is working
  - [x] Gestures are working
- [x] Rendering
  - [x] Slice based 2D renderer

# Next Steps

## ESP32

- Script language
  - [x] host binding functions need to be written
- TCP client download plugin (can download directly in PSRAM)
  - [x] Receive Sprite Pack in PSRAM           (Max size:  4 MB)
  - [x] Receive Font Pack in SRAM?             (Max size: 64 kB)
  - [x] Receive Palette Pack in SRAM?          (Max size:  8 kB, ~16 palettes)
  - [x] Receive Script in SRAM?                (Max size: 64 kB, or 32 kB?)
  - [ ] Receive House Meta Data in SRAM?       (Max size:  2 kB)
  - [ ] Receive House Global Data in SRAM?     (Max size:  2 kB)
  - [ ] Receive House Sensor Data in SRAM?     (Max size:  2 kB)
- Rendering
  - [ ] Draw sprite with scaling support
  - [ ] Screen Slice                                            (Max size: 60 kB (60 x 480 x 2))
  - [ ] 2 Command buffers for double buffered rendering in SRAM (Max size:  8 kB (2 * 4 kB))
  - [ ] Build command buffer from draw calls
  - [ ] Compare command buffers and only execute when different from last frame or if
        frame buffer has been changed by other means.
  - [ ] Update frame-buffer by executing the command buffer
  - [ ] Swap command-buffers 
