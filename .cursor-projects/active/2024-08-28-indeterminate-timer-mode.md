# Project: Indeterminate Timer Mode

- **Created**: 2024-08-28
- **Status**: Active
- **Last Updated**: 2024-08-28

## Context & Requirements

The Focus Dial currently supports countdown timers set in 5-minute increments. This project introduces an "Indeterminate Timer Mode" which functions as a count-up timer (stopwatch).

**Requirements:**

- Allow the user to set the timer duration to 0 minutes using the rotary encoder in the Adjust state.
- When the duration is set to 0, the display in the Adjust state should indicate this mode (e.g., show "--:--").
- When a timer is started with a 0 duration:
  - The Timer state should count _up_ from 00:00:00.
  - The display should reflect the elapsed time counting up.
  - The timer continues until manually stopped by the user (button press).
- When the timer is stopped:
  - The Done state should display the final elapsed time.
  - The webhook payload should correctly report the elapsed time (`duration_set_minutes` = 0, `duration_actual_seconds` = elapsed seconds).

## Development Plan

### Phase 1: Adjust State Modification

- [x] Modify `AdjustState` logic to allow setting the duration to 0 minutes via the rotary encoder.
- [x] Update `DisplayController::drawAdjustScreen` to display "--:--" when the pending duration is 0.
- [x] Ensure `StateMachine` correctly stores and passes the 0 duration value to `TimerState` upon starting. (Verified existing mechanism sufficient)

### Phase 2: Timer State & Display Modification

- [x] Modify `TimerState::enter` to handle the 0-duration case (set solid LED color).
- [x] Modify `TimerState::update` to include conditional logic for count-up vs countdown.
- [x] Update `DisplayController::drawTimerScreen` to format and display time counting up or down based on a flag.
- [x] Ensure the timer stops correctly via button press in indeterminate mode (goes to DoneState).

### Phase 3: Done State & Webhook Integration

- [x] Update `DisplayController::drawDoneScreen` to correctly display the final elapsed time for indeterminate sessions.
- [x] Verify `NetworkController::sendWebhookAction` sends the correct payload (`duration_set_minutes`, `duration_actual_seconds`) for indeterminate sessions.

### Phase 4: Testing & Refinement

- [ ] Build and flash firmware with changes.
- [ ] Test setting duration to 0 in `AdjustState`.
- [ ] Test starting, running (observing count-up), and stopping an indeterminate timer.
- [ ] Test display updates in `AdjustState`, `TimerState`, and `DoneState` for indeterminate mode.
- [ ] Test webhook payload accuracy using a test endpoint.
- [ ] Test edge cases (e.g., stopping immediately after starting, double-press cancel).

## Notes & References

- Reviewed `AdjustState.cpp`, `TimerState.cpp`, `DoneState.cpp`, `PausedState.cpp`, `DisplayController.cpp`, `NetworkController.cpp`, `StateMachine.h`, `StateMachine.cpp`.
- Added `setPendingElapsedTime`/`getPendingElapsedTime` to `StateMachine`.
- Updated signatures for `drawAdjustScreen`, `drawTimerScreen`, `drawDoneScreen`, `sendWebhookAction`.
- LED behavior for indeterminate mode is solid color.
