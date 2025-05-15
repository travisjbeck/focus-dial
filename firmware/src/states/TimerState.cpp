#include "StateMachine.h"
#include "Controllers.h"
#include "managers/ProjectManager.h" // Required for getProjectManagerInstance
#include <lvgl.h>                  // For LVGL objects

// Static LVGL event handlers
static void timer_screen_tap_event_handler(lv_event_t *e);
static void timer_screen_long_press_event_handler(lv_event_t *e);

TimerState::TimerState() : 
    duration(0), 
    elapsedTime(0), 
    startTime(0), 
    currentProjectId(""), // Initialize currentProjectId
    currentProjectName("No Project"), // Default
    currentLedColor(0),
    projectNameLabel(nullptr),
    timeDisplayLabel(nullptr),
    timerProgressBar(nullptr) 
{
    // No need to initialize currentProjectName and currentLedColor here
}

void TimerState::setCurrentProjectDetails(const String& id, const String& name, uint32_t color) {
    this->currentProjectId = id;
    this->currentProjectName = name;
    this->currentLedColor = color;
    Serial.printf("TimerState::setCurrentProjectDetails - ID: %s, Name: %s, Color: %06X\n",
                  this->currentProjectId.c_str(), this->currentProjectName.c_str(), this->currentLedColor);
}

void TimerState::processScreenTap() {
    Serial.println("TimerState: Screen Tapped");
    networkController.sendWebhookAction("stop", this->duration, this->elapsedTime);

    if (this->duration == 0) { // Indeterminate mode
        Serial.println("TimerState: Stopping Indeterminate Timer, going to DoneState");
        stateMachine.setPendingElapsedTime(this->elapsedTime);
        stateMachine.setPendingProjectId(this->currentProjectId); // Pass current project ID
        stateMachine.changeState(&StateMachine::doneState);
    } else { // Countdown mode
        Serial.println("TimerState: Pausing Countdown Timer, going to PausedState");
        // Pass currentProjectId to PausedState
        StateMachine::pausedState.setPause(this->currentProjectId, this->duration, this->elapsedTime, this->currentLedColor, this->currentProjectName);
        stateMachine.changeState(&StateMachine::pausedState);
    }
}

void TimerState::processScreenLongPress() {
    Serial.println("TimerState: Screen Long Pressed - Canceling");
    networkController.sendWebhookAction("stop", this->duration, this->elapsedTime);
    stateMachine.changeState(&StateMachine::idleState);
}

static void timer_screen_tap_event_handler(lv_event_t *e) {
    TimerState* self = (TimerState*)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && self) {
        if (millis() - self->getEntryTime() < State::TAP_DEBOUNCE_MS) {
            Serial.println("TimerState: Tap ignored (debounce)");
            return;
        }
        self->processScreenTap();
    }
}

static void timer_screen_long_press_event_handler(lv_event_t *e) {
    TimerState* self = (TimerState*)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED && self) {
        self->processScreenLongPress();
    }
}

void TimerState::enter()
{
  State::enter(); 
  Serial.println("Entering Timer State");
  startTime = millis() - (elapsedTime * 1000); 

  if (elapsedTime == 0) { // Initial entry (not resuming from pause)
    Serial.println("Timer State: Initial entry for timer.");
    this->currentProjectId = stateMachine.getPendingProjectId(); // Get ID from StateMachine
    String projectColorHex = "#FFFFFF"; 
    
    if (!this->currentProjectId.isEmpty()) {
      bool found = false;
      const auto &allProjects = getProjectManagerInstance().getProjects();
      for (const auto &p : allProjects) {
        if (p.device_project_id == this->currentProjectId) {
          this->currentProjectName = p.name;
          projectColorHex = p.color;
          found = true;
          break;
        }
      }
      if (!found) {
          Serial.printf("TimerState: Project ID '%s' not found, defaulting.\n", this->currentProjectId.c_str());
          this->currentProjectName = "No Project";
          this->currentProjectId = ""; // Clear ID if not found
      } else {
          Serial.printf("TimerState: Project '%s' (ID: %s) loaded.\n", this->currentProjectName.c_str(), this->currentProjectId.c_str());
      }
    } else {
      this->currentProjectName = "No Project";
      Serial.println("TimerState: No pending project ID, using 'No Project'.");
    }
    this->currentLedColor = LEDController::hexColorToUint32(projectColorHex);
  } else { 
    // Resuming from pause: currentProjectId, currentProjectName, currentLedColor 
    // should have been set by setCurrentProjectDetails via PausedState.
    Serial.printf("Timer State: Resuming. Project: %s (ID: %s), Color: %06X\n", 
                  currentProjectName.c_str(), currentProjectId.c_str(), currentLedColor);
  }

  if (this->duration == 0) {
    Serial.println("TimerState: Indeterminate mode. Starting breathing LED effect.");
    ledController.setBreath(currentLedColor, -1, false, 5); 
  } else {
    uint32_t remainingDurationMs = (this->duration * 60 - this->elapsedTime) * 1000;
    if (remainingDurationMs > 0) { ledController.startFillAndDecay(currentLedColor, remainingDurationMs); }
    else { ledController.turnOff(); }
  }

  lv_obj_t *screen = lv_screen_active();
  if (!screen) { Serial.println("TimerState::enter() - FATAL: screen is NULL"); return; }
  lv_obj_clean(screen);

  projectNameLabel = lv_label_create(screen);
  if(projectNameLabel) {
    lv_label_set_text(projectNameLabel, currentProjectName.c_str());
    lv_obj_set_style_text_font(projectNameLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(projectNameLabel, LV_ALIGN_TOP_MID, 0, 15);
  } else { Serial.println("TimerState: projectNameLabel creation FAILED"); }

  timeDisplayLabel = lv_label_create(screen);
  if(timeDisplayLabel) {
    lv_obj_set_style_text_font(timeDisplayLabel, &lv_font_montserrat_14, 0); 
    char timeStr[10];
    int initialDisplaySeconds = (duration == 0) ? elapsedTime : (duration * 60 - elapsedTime);
    if (initialDisplaySeconds < 0) initialDisplaySeconds = 0;
    int hours = initialDisplaySeconds / 3600;
    int minutes = (initialDisplaySeconds % 3600) / 60;
    int seconds = initialDisplaySeconds % 60;
    if (hours > 0) { sprintf(timeStr, "%02d:%02d", hours, minutes); } 
    else { sprintf(timeStr, "%02d:%02d", minutes, seconds); }
    lv_label_set_text(timeDisplayLabel, timeStr);
    lv_obj_align(timeDisplayLabel, LV_ALIGN_CENTER, 0, 0);
  } else { Serial.println("TimerState: timeDisplayLabel creation FAILED"); }
  
  if (duration > 0) { 
      timerProgressBar = lv_bar_create(screen);
      if(timerProgressBar) {
        lv_obj_set_size(timerProgressBar, lv_pct(80), 10);
        lv_obj_align(timerProgressBar, LV_ALIGN_BOTTOM_MID, 0, -30);
        lv_bar_set_range(timerProgressBar, 0, duration * 60);
        lv_bar_set_value(timerProgressBar, elapsedTime, LV_ANIM_OFF); 
      } else { Serial.println("TimerState: timerProgressBar creation failed"); }
  }

  lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(screen, timer_screen_tap_event_handler, LV_EVENT_CLICKED, this);
  lv_obj_add_event_cb(screen, timer_screen_long_press_event_handler, LV_EVENT_LONG_PRESSED, this);
  Serial.println("TimerState: Screen CLICK and LONG PRESS events ADDED.");

  if (elapsedTime == 0) {
    networkController.sendWebhookAction("start", this->duration, 0);
  }
  Serial.println("TimerState::enter - finished.");
}

void TimerState::update()
{
  inputController.update();
  ledController.update(); 

  unsigned long currentTime = millis();
  elapsedTime = (currentTime - startTime) / 1000;
  int displaySeconds;
  char timeStr[10]; 

  if (duration == 0) { 
    displaySeconds = elapsedTime;
    int hours = displaySeconds / 3600;
    int minutes = (displaySeconds % 3600) / 60;
    int seconds = displaySeconds % 60;
    if (hours > 0) { sprintf(timeStr, "%02d:%02d", hours, minutes); }
    else { sprintf(timeStr, "%02d:%02d", minutes, seconds); }
    if (timeDisplayLabel && lv_obj_is_valid(timeDisplayLabel)) {
        lv_label_set_text(timeDisplayLabel, timeStr);
    }
  } else { 
    displaySeconds = duration * 60 - elapsedTime;
    if (displaySeconds < 0) displaySeconds = 0;
    int hours = displaySeconds / 3600;
    int minutes = (displaySeconds % 3600) / 60;
    int seconds = displaySeconds % 60;
    if (hours > 0) { sprintf(timeStr, "%02d:%02d", hours, minutes); }
    else { sprintf(timeStr, "%02d:%02d", minutes, seconds); }
    if (timeDisplayLabel && lv_obj_is_valid(timeDisplayLabel)) {
        lv_label_set_text(timeDisplayLabel, timeStr);
    }

    if (timerProgressBar && lv_obj_is_valid(timerProgressBar)) {
        int current_progress = elapsedTime;
        if (current_progress > duration * 60) current_progress = duration * 60;
        lv_bar_set_value(timerProgressBar, current_progress, LV_ANIM_OFF);
    }

    if (displaySeconds <= 0) {
      Serial.println("Timer State: Done (Countdown), going to DoneState");
      stateMachine.setPendingElapsedTime(this->duration * 60);
      stateMachine.setPendingProjectId(this->currentProjectId); // Pass current project ID
      stateMachine.changeState(&StateMachine::doneState); 
    }
  }
}

void TimerState::exit()
{
  Serial.println("Exiting Timer State");
  lv_obj_t *screen = lv_screen_active();
  if (screen) { 
    lv_obj_remove_event_cb_with_user_data(screen, timer_screen_tap_event_handler, this);
    lv_obj_remove_event_cb_with_user_data(screen, timer_screen_long_press_event_handler, this);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE); 
  }
  // LVGL objects are children of the screen and should be cleaned by the next state's enter() or explicitly here.
  // For safety, explicitly delete them.
  if (projectNameLabel) { lv_obj_del_async(projectNameLabel); projectNameLabel = nullptr; }
  if (timeDisplayLabel) { lv_obj_del_async(timeDisplayLabel); timeDisplayLabel = nullptr; }
  if (timerProgressBar) { lv_obj_del_async(timerProgressBar); timerProgressBar = nullptr; }
  Serial.println("TimerState: LVGL objects scheduled for deletion.");
}

void TimerState::setTimer(int durationMinutes, unsigned long elapsedSeconds)
{
  this->duration = durationMinutes;
  this->elapsedTime = elapsedSeconds;
  Serial.printf("TimerState::setTimer called - Duration: %d min, Elapsed: %lu sec\n", this->duration, this->elapsedTime);
}

// PausedState will also need a way to pass back currentLedColor and currentProjectName