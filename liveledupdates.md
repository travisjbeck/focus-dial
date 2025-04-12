# Live LED Updates Feature Plan

## Overview
Add real-time LED ring updates on the Focus Dial device when selecting colors in the configuration website's project color picker. This provides immediate visual feedback on how colors will look on the device.

## Prerequisites
- Focus Dial firmware with working configuration web interface
- LEDController already implemented and functional
- WebServer already set up and serving the configuration page
- Idle state working correctly

## Technical Approach
We'll implement this using WebSockets to create a real-time communication channel between the web UI and the Focus Dial. When users interact with the color picker, we'll send color updates to the device, which will temporarily change the LED ring color if the device is in the Idle state.

## Detailed Tasks

### 1. Add WebSocket Support to ESP32 Server
- [x] Check if ESPAsyncWebServer WebSocket support is already included
- [x] Update PlatformIO dependencies if needed
- [x] Create a WebSocket handler in NetworkController
- [x] Add WebSocket event handlers (connect, disconnect, message)
- [x] Implement a "preview-color" message type handler
- [x] Implement a "reset-color" message type handler
- [x] Build and monitor output

### 2. Add LED Preview Mode to LEDController
- [x] Add a previewMode flag to LEDController
- [x] Create setPreviewColor(String hexColor) method
- [x] Create resetPreviewColor() method to restore previous color
- [x] Ensure LED state is properly restored after preview
- [x] Build and monitor output

### 3. Modify StateMachine to Handle Color Preview
- [x] Add method to check if currently in IdleState
- [x] Create a method to handle color preview requests
- [x] Only allow color preview when in IdleState
- [x] Build and monitor output

### 4. Add WebSocket Client to Web UI
- [x] Add WebSocket connection code to the web UI
- [x] Create connection open/close handlers
- [x] Add error handling for WebSocket connection
- [x] Build and monitor output

### 5. Connect Color Picker to WebSocket
- [x] Add event listeners to the color picker input
- [x] Send color updates via WebSocket on change/input events
- [x] Add debounce to avoid flooding the device with updates
- [x] Send "reset-color" when closing color picker or cancelling
- [x] Build and monitor output

### 6. Test and Debug
- [x] Build the firmware
- [x] Upload firmware and filesystem to the device
- [x] Test color picker in idle state
- [x] Test color picker in non-idle states (should not update LEDs)
- [x] Test connection interruptions
- [x] Test multiple connected clients
- [x] Verify color resets properly after selection
- [x] Verify logs and debug messages

### 7. Optimize Performance
- [x] Review WebSocket message frequency
- [x] Consider adding throttling if needed
- [x] Test on slow connections
- [x] Final build and verification

## Implementation Summary
✅ **COMPLETE:** The Live LED Updates feature has been successfully implemented and tested!

We have implemented:
1. WebSocket server on the ESP32 that listens for color preview requests
2. LED preview mode in the LEDController to temporarily show colors
3. State management to only allow color previews in the idle state
4. WebSocket client in the web UI with event handlers for color picker changes
5. Debounce mechanism to prevent flooding the device with updates

The feature now allows users to see real-time feedback on the Focus Dial's LED ring while picking colors for projects in the web interface. Testing has confirmed that:
- Color updates in real-time as the user moves the color picker
- LEDs revert to their original state when the color picker is closed
- The feature only works when the device is in idle state
- Multiple connected clients handle the WebSocket connection properly

## Code Locations

### Firmware Files Modified:
- `firmware/src/controllers/NetworkController.cpp` - Added WebSocket handlers
- `firmware/include/controllers/NetworkController.h` - Added WebSocket method declarations
- `firmware/src/controllers/LEDController.cpp` - Added preview methods
- `firmware/include/controllers/LEDController.h` - Added preview method declarations
- `firmware/src/StateMachine.cpp` - Added helper methods for state checking
- `firmware/include/StateMachine.h` - Added method declarations

### Web UI Files Modified:
- `firmware/data/app.js` - Added WebSocket client code

## Notes and Considerations
- WebSocket updates are lightweight with 50ms debounce to prevent performance issues
- State management ensures original LED state is always restored
- Mobile browsers handle the color picker and WebSockets correctly
- Clear error handling ensures graceful recovery from connection interruptions
- Multiple clients can connect to the config page simultaneously without conflicts 