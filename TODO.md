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
  - host binding functions need to be written
- WiFi downloading (need to refactor a bit to receive AssetDb and Script)
  - Receive AssetDb in PSRAM
  - Receive Script in SRAM
- Rendering
  - Draw sprite needs to accept blend alpha
  - Need to build command buffer from draw calls
  - Need to render frame-buffer from command buffer
  - Need to finalize double buffered rendering
