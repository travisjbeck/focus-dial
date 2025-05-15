#include "StateMachine.h"
#include "Controllers.h"
#include "managers/ProjectManager.h" // Potentially for project name display if needed
#include <lvgl.h>                  // For LVGL objects

// Static event handlers for PausedState (if needed for its own UI later)
// static void paused_screen_tap_event_handler(lv_event_t *e);
// static void paused_screen_long_press_event_handler(lv_event_t *e);

PausedState::PausedState() : 
    duration(0), 
    elapsedTimeAtPause(0), // Renamed from elapsedTime for clarity
    pauseEnter(0),
    pausedLedColor(0),
    pausedProjectName("No Project"),
    pausedTimeLabel(nullptr),
    pausedProjectNameLabel(nullptr),
    instructionLabel(nullptr)
{}

// Public helper methods for LVGL events
void PausedState::processScreenTap() {
    Serial.println("Paused State: Screen Tapped - Resuming");
    // Send 'start' action to webhook handler (resume)
    networkController.sendWebhookAction("start", this->duration, this->elapsedTimeAtPause); 

    StateMachine::timerState.setTimer(duration, elapsedTimeAtPause); 
    // Pass back color and name to TimerState if they were stored from TimerState
    // This requires TimerState::setTimer to accept them, or TimerState::enter to re-fetch if only ID is passed.
    // For now, TimerState::enter handles re-fetching color/name if elapsedTime is 0,
    // but on resume, it reuses its own stored currentLedColor & currentProjectName.
    // We need to ensure TimerState can be updated with these upon resume.
    // One way: TimerState could have a resumeTimer(color, name) method, 
    // or setTimer could be enhanced. 
    // Let's assume for now TimerState will correctly use its own currentLedColor/Name upon resume if elapsedTime > 0.
    
    // displayController.showTimerResume(); // TODO: LVGL animation
    stateMachine.changeState(&StateMachine::timerState); 
}

void PausedState::processScreenLongPress() {
    Serial.println("Paused State: Screen Long Pressed - Canceling");
    networkController.sendWebhookAction("stop", this->duration, this->elapsedTimeAtPause);
    // displayController.showCancel(); // TODO: LVGL animation
    stateMachine.changeState(&StateMachine::idleState); 
}

// Static event handlers linking to public methods
// static void paused_screen_tap_event_handler(lv_event_t *e) {
//     PausedState* self = (PausedState*)lv_event_get_user_data(e);
//     if (lv_event_get_code(e) == LV_EVENT_CLICKED && self) {
//         self->processScreenTap();
//     }
// }

// static void paused_screen_long_press_event_handler(lv_event_t *e) {
//     PausedState* self = (PausedState*)lv_event_get_user_data(e);
//     if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED && self) {
//         self->processScreenLongPress();
//     }
// }

void PausedState::enter()
{
  Serial.println("Entering Paused State");
  pauseEnter = millis(); 
  ledController.setBreath(FD_YELLOW, -1, false, 20); // Yellow breathing for paused

  // For now, PausedState will use the placeholder screen from DisplayController
  // The actual UI (labels for paused time, project name, instructions) will be built here in a later step.
  // When we build the UI, we will also add the tap/long-press handlers to the screen like in other states.
  int remainingTime = (duration * 60) - elapsedTimeAtPause;
  if (remainingTime < 0) remainingTime = 0;
  displayController.drawPausedScreen(remainingTime); // Uses placeholder for now

  // TODO: Setup LVGL UI for PausedState here
  // Example:
  // lv_obj_t *screen = lv_screen_active();
  // lv_obj_clean(screen);
  // pausedProjectNameLabel = lv_label_create(screen, ...);
  // lv_label_set_text(pausedProjectNameLabel, pausedProjectName.c_str());
  // pausedTimeLabel = lv_label_create(screen, ...);
  // lv_label_set_text_fmt(pausedTimeLabel, "%02d:%02d", minutes, seconds);
  // instructionLabel = lv_label_create(screen, ...);
  // lv_label_set_text(instructionLabel, "Tap to Resume\nLong Press to Cancel");
  // lv_obj_add_event_cb(screen, paused_screen_tap_event_handler, LV_EVENT_CLICKED, this);
  // lv_obj_add_event_cb(screen, paused_screen_long_press_event_handler, LV_EVENT_LONG_PRESSED, this);
  // lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
  // lv_refr_now(NULL);
}

void PausedState::update()
{
  inputController.update(); // Though no handlers are set for PausedState yet
  ledController.update();   // For breathing animation

  // If not building specific UI in enter(), keep calling drawPausedScreen for placeholder
  // Once specific UI is built in enter(), this call might not be needed or would update specific elements.
  int remainingTime = (duration * 60) - elapsedTimeAtPause;
  if (remainingTime < 0) remainingTime = 0;
  displayController.drawPausedScreen(remainingTime); // Uses placeholder for now

  unsigned long currentTime = millis();
  if (currentTime - pauseEnter >= (PAUSE_TIMEOUT * 60 * 1000)) {
    Serial.println("Paused State: Timeout - Canceling timer");
    networkController.sendWebhookAction("stop", this->duration, this->elapsedTimeAtPause);
    // displayController.showCancel(); // TODO: LVGL animation
    stateMachine.changeState(&StateMachine::idleState); 
  }
}

void PausedState::exit()
{
  Serial.println("Exiting Paused State");
  // inputController.releaseHandlers(); // No handlers registered in this version of enter yet
  
  // TODO: Clean up LVGL objects if created in enter()
  // lv_obj_t *screen = lv_screen_active();
  // if (screen) {
  //   lv_obj_remove_event_cb_with_user_data(screen, paused_screen_tap_event_handler, this);
  //   lv_obj_remove_event_cb_with_user_data(screen, paused_screen_long_press_event_handler, this);
  //   lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE);
  // }
  // if(pausedTimeLabel) { lv_obj_del(pausedTimeLabel); pausedTimeLabel = nullptr; }
  // if(pausedProjectNameLabel) { lv_obj_del(pausedProjectNameLabel); pausedProjectNameLabel = nullptr; }
  // if(instructionLabel) { lv_obj_del(instructionLabel); instructionLabel = nullptr; }
}

// Updated to accept and store color and project name
void PausedState::setPause(int dur, unsigned long elapsed, uint32_t ledCol, const String& projName)
{
  this->duration = dur;
  this->elapsedTimeAtPause = elapsed; // Store the elapsed time at the moment of pausing
  this->pausedLedColor = ledCol;
  this->pausedProjectName = projName;
  Serial.printf("PausedState::setPause - Duration: %d, ElapsedAtPause: %lu, Color: %06X, Project: %s\n", 
                this->duration, this->elapsedTimeAtPause, this->pausedLedColor, this->pausedProjectName.c_str());
}