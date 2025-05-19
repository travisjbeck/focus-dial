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
    currentLedColor(0),
    projectNameLabel(nullptr),
    timeDisplayLabel(nullptr),
    timerProgressBar(nullptr) 
{
    currentProjectName = "No Project"; // Default
}

void TimerState::processScreenTap() {
    Serial.println("TimerState: Screen Tapped");
    networkController.sendWebhookAction("stop", this->duration, this->elapsedTime);

    if (this->duration == 0) { // Indeterminate mode
        Serial.println("TimerState: Stopping Indeterminate Timer");
        stateMachine.setPendingElapsedTime(this->elapsedTime);
        stateMachine.changeState(&StateMachine::doneState);
    } else { // Countdown mode
        Serial.println("TimerState: Pausing Countdown Timer");
        StateMachine::pausedState.setPause(this->duration, this->elapsedTime, this->currentLedColor, this->currentProjectName);
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
        if (millis() - self->getEntryTime() < State::TAP_DEBOUNCE_MS) { // Use getter and State::
            Serial.println("TimerState: Tap ignored (debounce)");
            return;
        }
        self->processScreenTap();
    }
}

static void timer_screen_long_press_event_handler(lv_event_t *e) {
    TimerState* self = (TimerState*)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED && self) {
        // Long press might not need the same immediate debounce after state entry,
        // but consider if rapidly re-entering and long-pressing is an issue.
        // For now, no specific debounce beyond LVGL's own long-press time.
        self->processScreenLongPress();
    }
}

void TimerState::enter()
{
  State::enter(); 
  Serial.println("Entering Timer State (Testing CLICK Event Handler)");
  startTime = millis() - (elapsedTime * 1000); 

  // Determine currentProjectName and currentLedColor
  if (elapsedTime == 0) {
    Serial.println("Timer State: Initial entry");
    String pendingId = stateMachine.getPendingProjectId();
    String projectColorHex = "#FFFFFF"; 
    currentProjectName = "No Project";
    if (!pendingId.isEmpty()) {
      const auto &allProjects = getProjectManagerInstance().getProjects();
      for (const auto &p : allProjects) {
        if (p.device_project_id == pendingId) {
          currentProjectName = p.name;
          projectColorHex = p.color;
          break;
        }
      }
    }
    currentLedColor = LEDController::hexColorToUint32(projectColorHex);
  } else { 
    Serial.printf("Timer State: Resuming. Project: %s, Color: %06X\n", currentProjectName.c_str(), currentLedColor);
  }

  // LED setup logic (radar sweep still commented for duration 0)
  if (this->duration == 0) {
    Serial.println("TimerState: Indeterminate mode. Starting breathing LED effect.");
    ledController.setBreath(currentLedColor, -1, false, 5); // Breathe with project color (or default white)
  } else {
    uint32_t remainingDurationMs = (this->duration * 60 - this->elapsedTime) * 1000;
    if (remainingDurationMs > 0) { ledController.startFillAndDecay(currentLedColor, remainingDurationMs); }
    else { ledController.turnOff(); }
  }

  lv_obj_t *screen = lv_screen_active();
  if (!screen) { Serial.println("TimerState::enter() - FATAL: screen is NULL"); return; }
  lv_obj_clean(screen);
  Serial.println("TimerState: Screen cleaned.");

  // Create projectNameLabel
  projectNameLabel = lv_label_create(screen);
  Serial.printf("TimerState::enter - projectNameLabel pointer: %p\n", (void*)projectNameLabel);
  if(projectNameLabel) {
    lv_label_set_text(projectNameLabel, currentProjectName.c_str());
    lv_obj_set_style_text_font(projectNameLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(projectNameLabel, LV_ALIGN_TOP_MID, 0, 15);
    Serial.println("TimerState::enter - projectNameLabel created and styled.");
  } else { Serial.println("TimerState: projectNameLabel creation FAILED"); }

  // Create timeDisplayLabel
  timeDisplayLabel = lv_label_create(screen);
  Serial.printf("TimerState::enter - timeDisplayLabel pointer: %p\n", (void*)timeDisplayLabel);
  if(timeDisplayLabel) {
    lv_obj_set_style_text_font(timeDisplayLabel, &lv_font_montserrat_14, 0); 
    // Let's use the dynamic text setting now that we suspect event handlers
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
    Serial.println("TimerState::enter - timeDisplayLabel created with font and DYNAMIC initial text.");
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

  // RE-ADDING CLICK EVENT HANDLER ONLY
  lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(screen, timer_screen_tap_event_handler, LV_EVENT_CLICKED, this);
  // lv_obj_add_event_cb(screen, timer_screen_long_press_event_handler, LV_EVENT_LONG_PRESSED, this); // Keep long press commented
  Serial.println("TimerState: Screen CLICK event ADDED. Long press NOT added.");

  if (elapsedTime == 0) {
    networkController.sendWebhookAction("start", this->duration, 0);
  }
  // lv_refr_now(NULL); // Force an immediate redraw - Potentially problematic
  Serial.println("TimerState::enter - finished (CLICK Event Handler Test).");
}

void TimerState::update()
{
  // Restore dynamic text update for timeDisplayLabel
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
    if (hours > 0) {
        sprintf(timeStr, "%02d:%02d", hours, minutes);
    } else {
        sprintf(timeStr, "%02d:%02d", minutes, seconds);
    }
    if (timeDisplayLabel && lv_obj_is_valid(timeDisplayLabel)) {
        lv_label_set_text(timeDisplayLabel, timeStr);
    } else if (timeDisplayLabel) { 
        Serial.println("TimerState::update - timeDisplayLabel exists but is invalid (count up)");
    } else { 
        Serial.println("TimerState::update - timeDisplayLabel is NULL (count up)");
    }
  } else { 
    displaySeconds = duration * 60 - elapsedTime;
    if (displaySeconds < 0) displaySeconds = 0;
    int hours = displaySeconds / 3600;
    int minutes = (displaySeconds % 3600) / 60;
    int seconds = displaySeconds % 60;
    if (hours > 0) {
        sprintf(timeStr, "%02d:%02d", hours, minutes);
    } else {
        sprintf(timeStr, "%02d:%02d", minutes, seconds);
    }
    if (timeDisplayLabel && lv_obj_is_valid(timeDisplayLabel)) {
        lv_label_set_text(timeDisplayLabel, timeStr);
    } else if (timeDisplayLabel) { 
        Serial.println("TimerState::update - timeDisplayLabel exists but is invalid (count down)");
    } else { 
        Serial.println("TimerState::update - timeDisplayLabel is NULL (count down)");
    }

    // Progress bar logic (still not created in enter for this test)
    // if (timerProgressBar && lv_obj_is_valid(timerProgressBar)) { ... }

    if (displaySeconds <= 0) {
      Serial.println("Timer State: Done (Countdown)");
      stateMachine.setPendingElapsedTime(this->duration * 60);
      stateMachine.changeState(&StateMachine::doneState); 
    }
  }
}

void TimerState::exit()
{
  Serial.println("Exiting Timer State (CLICK Event Handler Test)");
  
  lv_obj_t *screen = lv_screen_active();
  if (screen) { 
    lv_obj_remove_event_cb_with_user_data(screen, timer_screen_tap_event_handler, this);
    // lv_obj_remove_event_cb_with_user_data(screen, timer_screen_long_press_event_handler, this); // Keep commented
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE); // Still clear flag if it was added
    Serial.println("TimerState: Screen CLICK event removed.");
  }

  if (projectNameLabel) { lv_obj_del(projectNameLabel); projectNameLabel = nullptr; }
  if (timeDisplayLabel) { lv_obj_del(timeDisplayLabel); timeDisplayLabel = nullptr; }
  if (timerProgressBar) { lv_obj_del(timerProgressBar); timerProgressBar = nullptr; }
  Serial.println("TimerState: LVGL objects cleaned.");
}

void TimerState::setTimer(int durationMinutes, unsigned long elapsedSeconds)
{
  this->duration = durationMinutes;
  this->elapsedTime = elapsedSeconds;
  // Initial color and name will be set in enter() if elapsedTime is 0
  Serial.printf("TimerState::setTimer called - Duration: %d min, Elapsed: %lu sec\n", this->duration, this->elapsedTime);
}

// PausedState will also need a way to pass back currentLedColor and currentProjectName
// when resuming. Modify PausedState::setPause and TimerState::enter for this.