# Gui state machine

## Connect to WiFi

## Connect to Asset Server

- TCP: Wait until received:
  - Receive Asset Db (comes in chunks)
  - Receive Script
- UDP: Wait until received the first update message for home state

## Initialize Hardware

- Initialize LCD
- Initialize Touch
- Initialize SD Card
- Initialize Sensors

## Initialize Software

- Initialize Asset Db
- Prepare script VM for execution

## Execute/Tick

- Read Touch and Touch Gestures
- Receive UDP messages
  - update home state
- Update Time and Date
- Update Date and Time String
- Execute script VM
- Render GUI
- Swap frame buffers
- Local state change? -> send UDP message to Sensor Server
