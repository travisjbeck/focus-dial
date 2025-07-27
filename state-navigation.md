# Timer State Machine Navigation

## State Diagram

```
┌─────────────────┐
│   StartupState  │ (2 sec splash)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ ProvisionState  │ (First run only - WiFi setup)
└────────┬────────┘
         │ WiFi configured
         ▼
┌─────────────────┐ Rotate Encoder  ┌─────────────────┐
│                 │ ───────────────> │                 │
│   IdleState     │                  │  AdjustState    │
│   (25:00)       │ <─────────────── │ (Timer Duration)│
│                 │    Tap Screen    │                 │
└────────┬────────┘                  └─────────────────┘
         │ Tap Screen
         ▼
┌─────────────────┐
│ProjectSelectState│ 
│ (Choose Project) │
└────────┬────────┘
         │ Tap Screen (Start Timer)
         ▼
┌─────────────────┐ Tap Screen      ┌─────────────────┐
│                 │ ───────────────> │                 │
│   TimerState    │                  │  PausedState    │
│   (Running)     │ <─────────────── │   (Paused)      │
│                 │    Tap Screen    │                 │
└────────┬────────┘                  └────────┬────────┘
         │ Long Press                         │ Long Press
         ▼                                    ▼
┌─────────────────┐
│   DoneState     │
│  (Completed)    │
└────────┬────────┘
         │ Tap Screen
         ▼
    [Back to IdleState]
```

## Input Summary

### Rotary Encoder
- **IdleState**: Rotate to enter AdjustState (timer duration)
- **AdjustState**: Rotate to change timer duration (5-240 min)
- **ProjectSelectState**: Rotate to scroll through projects

### Screen Tap
- **IdleState**: Tap to enter ProjectSelectState
- **AdjustState**: Tap to save duration and return to IdleState
- **ProjectSelectState**: Tap to select project and start timer
- **TimerState**: Tap to pause
- **PausedState**: Tap to resume
- **DoneState**: Tap to return to IdleState

### Long Press
- **TimerState**: Long press to end timer early
- **PausedState**: Long press to end timer early

### Power Button (Hardware)
- **Any State**: Press to enter deep sleep
- **Deep Sleep**: Press BOOT button to wake

### Inactivity
- **IdleState**: 3 minutes → Light sleep (touch to wake)
- **Other States**: No timeout during active use

## Key Behaviors

1. **WiFi Check**: On every boot, check if WiFi is configured. Skip ProvisionState if already set up.

2. **Timer Modes**: Timer can count up or down (need to clarify which mode is used when).

3. **Project Colors**: Each project has an associated color that affects:
   - LED ring color during timer
   - UI accent colors
   - Progress indicators

4. **State Persistence**: 
   - Timer duration saved between sessions
   - Current project remembered
   - WiFi credentials stored in NVS

5. **Simplified Input**: Everything is either:
   - Single tap (most common - state transitions)
   - Long press (only in timer/paused states to end timer)
   - Encoder rotation (value adjustments)

## Implementation Notes

1. **Touch Debouncing**: Already implemented in TheTimerArduino
2. **State Transition Animation**: Fade transitions already working
3. **Encoder Interrupts**: Need to implement on GPIO 17/18
4. **Long Press Detection**: Need to add to LVGL event handling
5. **WiFi Check**: Need to port from old firmware's NetworkController

## Testing Each Transition

1. Boot → Provision → Idle (first run)
2. Boot → Idle (subsequent runs)
3. Idle ↔ Adjust (encoder/tap cycle)
4. Idle → Project → Timer (tap flow)
5. Timer ↔ Paused (tap toggle)
6. Timer/Paused → Done (long press)
7. Done → Idle (tap)
8. Any → Sleep → Wake (power/inactivity)