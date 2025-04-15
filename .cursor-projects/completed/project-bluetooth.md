# Project Task: Investigate and Fix Bluetooth Trigger

## 1. Problem Description

- **Symptom:** Device crashes shortly after entering the `TimerState`.
- **Logs:** Serial monitor shows `E (####) A2DP: i2s_set_clk failed` errors, followed by other Bluetooth/A2DP related errors, often culminating in an assertion failure or WDT reset.
- **Context:** This crash started occurring after integrating project selection and modifying state transitions. The original firmware intended to use Bluetooth connections to trigger iOS Automations (Focus Modes, HomeKit).

## 2. Investigation & Initial Fix

- **Analysis:** The crash pointed towards a conflict with the I2S peripheral used by the A2DP library. We hypothesized that the explicit calls to `networkController.startBluetooth()` in `TimerState::enter()` and `networkController.stopBluetooth()` in `TimerState::exit()` were conflicting with the background `bluetoothTask` managed by `NetworkController`, potentially trying to initialize or use the I2S driver while it was already configured or in use.
- **Action Taken:** Commented out the `startBluetooth()` and `stopBluetooth()` calls within `TimerState.cpp`.
- **Result:** This resolved the immediate crash, indicating the conflict was likely the cause.
- **Side Effect:** Disabling these calls also disabled the intended mechanism for triggering iOS automations based on the timer starting or stopping, as the Bluetooth connection state is no longer explicitly managed by the timer state.

## 3. Understanding the Bluetooth Trigger Mechanism

- The ESP32 is configured as an A2DP Sink (`BluetoothA2DPSink`).
- iOS Automations (Shortcuts, HomeKit) can be triggered based on a specific Bluetooth device connecting or disconnecting.
- The original firmware likely used the `startBluetooth()` call to make the ESP32 connectable/connect, triggering a "Connected" automation on the iPhone.
- The `stopBluetooth()` call would disconnect the device, triggering a "Disconnected" automation.

## 4. Next Steps / Future Investigation

The goal is to reinstate the Bluetooth connection/disconnection behavior tied to the timer state _without_ causing the I2S conflict and subsequent crash.

- **Option 1: Refine `NetworkController` Flags:**
  - Instead of calling `startBluetooth`/`stopBluetooth` directly from `TimerState`, modify `TimerState::enter` to set `networkController.bluetoothActive = true;` and `TimerState::exit` to set `networkController.bluetoothActive = false;`.
  - Review and potentially modify the `bluetoothTask` in `NetworkController.cpp` to reliably start/stop the A2DP sink (`a2dp_sink.start()` / `a2dp_sink.disconnect()`) based _only_ on the state of the `bluetoothActive` flag and the current connection status (`a2dp_sink.is_connected()`). Ensure no race conditions or repeated start attempts occur.
- **Option 2: Explicit I2S Management:**
  - Investigate if the `BluetoothA2DPSink` library requires explicit de-initialization (`a2dp_sink.end()`?) or if the underlying I2S driver needs specific handling (`i2s_driver_uninstall`?) before attempting to start/stop the A2DP service multiple times. This seems less likely if the library is intended to manage the driver internally, but worth checking documentation or examples.
- **Testing:**
  - After implementing a potential fix, thoroughly test starting/stopping timers multiple times, including pausing/resuming and cancelling, while monitoring logs for I2S errors or crashes.
  - Verify on an iOS device that the connection/disconnection events occur as expected when the timer starts and stops/finishes.
