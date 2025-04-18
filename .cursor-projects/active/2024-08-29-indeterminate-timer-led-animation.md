# Project: Indeterminate Timer LED Animation (Slow Fill/Wipe)
- **Created**: 2024-08-29
- **Status**: Planned
- **Last Updated**: 2024-08-29

## Context & Requirements
The goal is to add a distinct visual indicator using the Neopixel ring to show when the indeterminate timer (count-up mode) is actively running. This replaces the need for the breathing animation which is used in the idle state.

**Requirements:**
- When an indeterminate timer starts, the LED ring should begin a "Slow Fill/Wipe" animation using the selected project's color.
- The "Fill" phase should light up the ring segment by segment over approximately 60 seconds (configurable).
- The "Wipe" phase should turn off the ring segment by segment over approximately 60 seconds (configurable), following the fill direction.
- The animation should loop continuously (Fill -> Wipe -> Fill -> ...).
- The animation should pause when the timer is paused. A static, dim version of the project color on all LEDs is proposed for the paused state.
- The animation should stop and LEDs should turn off (or revert to idle state behavior) when the timer is stopped or exits the `TimerState`.
- This animation must *only* apply to the indeterminate timer mode (`_isCountUp == true`). Countdown timers should retain their existing LED behavior (likely fully lit ring).
- The animation update logic must be efficient and non-blocking.

## Development Plan
### Phase 1: LED Controller Implementation
- [ ] Define constants in `LEDController.h` or `Config.h` for animation (e.g., `FILL_WIPE_CYCLE_DURATION_MS = 120000`, `NUM_LEDS`). Assume `NUM_LEDS` is already available/defined.
- [ ] Add state variables to `LEDController.h` to manage the animation (e.g., `_animationMode`, `_animationStartTime`, `_animationColor`, `_isPaused`).
- [ ] Implement `LEDController::startFillWipeAnimation(uint32_t color)`: Sets mode, color, records start time.
- [ ] Implement `LEDController::updateFillWipeAnimation()`:
    - [ ] Calculate elapsed time within the current 120s cycle.
    - [ ] Determine if in Fill (0-60s) or Wipe (60-120s) phase.
    - [ ] Calculate the number of LEDs to light/darken based on progress through the current phase.
    - [ ] Update the `strip.pixels` buffer accordingly (light LEDs sequentially for Fill, darken for Wipe).
    - [ ] Call `strip.show()`.
- [ ] Implement `LEDController::pauseFillWipeAnimation()`: Sets `_isPaused` flag, records pause time (if needed for accurate resumption), sets LEDs to a static dim color.
- [ ] Implement `LEDController::resumeFillWipeAnimation()`: Clears `_isPaused`, adjusts `_animationStartTime` based on pause duration.
- [ ] Implement `LEDController::stopAnimation()`: Resets animation state variables (`_animationMode`), turns off LEDs.

### Phase 2: Timer State Integration & Testing
- [ ] Modify `TimerState::enter()`: If `_isCountUp`, call `ledController->startFillWipeAnimation(currentProject.color)`.
- [ ] Modify `TimerState::run()`:
    - [ ] If `_isCountUp` and timer *running*: Call `ledController->updateFillWipeAnimation()`.
    - [ ] If `_isCountUp` and timer *paused*: Call `ledController->pauseFillWipeAnimation()` (potentially only once on transition to pause).
    - [ ] If `_isCountUp` and timer *resumed*: Call `ledController->resumeFillWipeAnimation()` (potentially only once on transition to resume).
    - [ ] Ensure countdown timer (`!_isCountUp`) uses original LED logic (likely `setRingColor`).
- [ ] Modify `TimerState::exit()`: Call `ledController->stopAnimation()`.
- [ ] Build the firmware (`platformio run`).
- [ ] Flash firmware (`platformio run -t upload`).
- [ ] **Manual Testing:**
    - [ ] Start indeterminate timer: Verify fill animation starts (correct color, ~60s fill).
    - [ ] Observe loop: Verify wipe follows fill (~60s wipe), then fill restarts.
    - [ ] Pause timer during fill: Verify LEDs go static/dim.
    - [ ] Resume timer during fill: Verify fill resumes correctly.
    - [ ] Pause timer during wipe: Verify LEDs go static/dim.
    - [ ] Resume timer during wipe: Verify wipe resumes correctly.
    - [ ] Stop timer: Verify LEDs turn off.
    - [ ] Start countdown timer: Verify *no* fill/wipe animation occurs (ring likely stays fully lit).
    - [ ] Change projects and repeat indeterminate timer test to ensure color updates.

## Notes & References
- The number of LEDs on the ring needs to be confirmed/used (`NUM_LEDS`).
- Existing `LEDController` functions (`setRingColor`, etc.) should be reviewed to avoid conflicts.
- Consider using `millis()` for timing and calculating animation progress.
- Ensure integer division for LED index calculation is handled correctly. 