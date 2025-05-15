#include "StateMachine.h"
#include "Controllers.h"
#include "managers/ProjectManager.h" // Potentially for project name display if needed
#include <lvgl.h>                  // For LVGL objects
#include "states/PausedState.h" // Ensure this path is correct

// Static LVGL event handlers for PausedState
static void paused_screen_tap_event_handler(lv_event_t *e);
static void paused_screen_long_press_event_handler(lv_event_t *e);

PausedState::PausedState() : 
    activeProjectId(""),
    originalDurationMinutes(0),
    pausedElapsedTimeSeconds(0),
    activeLedColor(0),
    activeProjectName("Paused"),
    pausedLabel(nullptr),
    timeDisplayLabel(nullptr),
    projectNameLabel(nullptr) {}

void PausedState::setPause(const String& projectId, int totalDurationMinutes, unsigned long elapsedSeconds, uint32_t ledColor, const String& projectName) {
    this->activeProjectId = projectId;
    this->originalDurationMinutes = totalDurationMinutes;
    this->pausedElapsedTimeSeconds = elapsedSeconds;
    this->activeLedColor = ledColor;
    this->activeProjectName = projectName;
    Serial.printf("PausedState::setPause - ID: %s, Duration: %d min, Elapsed: %lu sec, Project: %s\n", 
                  activeProjectId.c_str(), originalDurationMinutes, pausedElapsedTimeSeconds, activeProjectName.c_str());
}

void PausedState::processResume() {
    Serial.println("PausedState: Resuming timer.");
    StateMachine::timerState.setTimer(this->originalDurationMinutes, this->pausedElapsedTimeSeconds);
    StateMachine::timerState.setCurrentProjectDetails(this->activeProjectId, this->activeProjectName, this->activeLedColor);
    stateMachine.changeState(&StateMachine::timerState);
}

void PausedState::processStop() {
    Serial.println("PausedState: Stopping timer (transition to Idle).");
    // Consider if a webhook for "cancel" or "stop" from pause is needed.
    // For now, just go to idle.
    networkController.sendWebhookAction("stop", this->originalDurationMinutes, this->pausedElapsedTimeSeconds);
    stateMachine.changeState(&StateMachine::idleState);
}

static void paused_screen_tap_event_handler(lv_event_t *e) {
    PausedState* self = (PausedState*)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && self) {
        if (millis() - self->getEntryTime() < State::TAP_DEBOUNCE_MS) {
            Serial.println("PausedState: Tap ignored (debounce)");
            return;
        }
        self->processResume();
    }
}

static void paused_screen_long_press_event_handler(lv_event_t *e) {
    PausedState* self = (PausedState*)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED && self) {
        self->processStop();
    }
}

void PausedState::enter() {
    State::enter();
    Serial.println("Entering Paused State");

    // LED indication for paused state - e.g., pulsing the active color slowly
    ledController.setPulse(activeLedColor, 1500, 1500); // Pulse active color, 1.5s on, 1.5s off

    lv_obj_t *screen = lv_screen_active();
    if (!screen) { Serial.println("PausedState::enter() - FATAL: screen is NULL"); return; }
    lv_obj_clean(screen);

    pausedLabel = lv_label_create(screen);
    lv_label_set_text(pausedLabel, "Timer Paused");
    lv_obj_set_style_text_font(pausedLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(pausedLabel, LV_ALIGN_TOP_MID, 0, 10);

    projectNameLabel = lv_label_create(screen);
    lv_label_set_text(projectNameLabel, activeProjectName.c_str());
    lv_obj_set_style_text_font(projectNameLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(projectNameLabel, LV_ALIGN_TOP_MID, 0, 35);

    timeDisplayLabel = lv_label_create(screen);
    char timeStr[10];
    int displaySeconds = (originalDurationMinutes == 0) ? pausedElapsedTimeSeconds : (originalDurationMinutes * 60 - pausedElapsedTimeSeconds);
    if (originalDurationMinutes > 0 && displaySeconds < 0) displaySeconds = 0; // Ensure countdown doesn't show negative if paused at exact end
    
    int hours = displaySeconds / 3600;
    int minutes = (displaySeconds % 3600) / 60;
    int seconds = displaySeconds % 60;
    if (originalDurationMinutes == 0) { // Count-up mode was paused
        if (hours > 0) sprintf(timeStr, "%02d:%02d", hours, minutes);
        else sprintf(timeStr, "%02d:%02d", minutes, seconds);
    } else { // Countdown mode was paused - show remaining time
        if (hours > 0) sprintf(timeStr, "%02d:%02d", hours, minutes); 
        else sprintf(timeStr, "%02d:%02d", minutes, seconds);
    }
    lv_label_set_text(timeDisplayLabel, timeStr);
    lv_obj_set_style_text_font(timeDisplayLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(timeDisplayLabel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* instructionLabel = lv_label_create(screen);
    lv_label_set_text(instructionLabel, "Tap to Resume\nLong Press to Stop");
    lv_obj_set_style_text_align(instructionLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(instructionLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(instructionLabel, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, paused_screen_tap_event_handler, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(screen, paused_screen_long_press_event_handler, LV_EVENT_LONG_PRESSED, this);
    
    Serial.println("PausedState::enter - UI created, event handlers added.");
}

void PausedState::update() {
    inputController.update(); 
    ledController.update(); 
    // No specific logic needed in update for PausedState as it's event-driven
}

void PausedState::exit() {
    Serial.println("Exiting Paused State");
    lv_obj_t *screen = lv_screen_active();
    if (screen) {
        lv_obj_remove_event_cb_with_user_data(screen, paused_screen_tap_event_handler, this);
        lv_obj_remove_event_cb_with_user_data(screen, paused_screen_long_press_event_handler, this);
        lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    }
    // LVGL objects are children of the screen, lv_obj_clean in enter() handles old ones.
    // No explicit lv_obj_del needed here if enter always cleans the screen.
    pausedLabel = nullptr;
    timeDisplayLabel = nullptr;
    projectNameLabel = nullptr;
}