#include "StateMachine.h"
#include "Controllers.h"
#include "Config.h" // For MIN_TIMER, MAX_TIMER
#include <lvgl.h>   // For LVGL objects

// Static LVGL event handler for screen tap
static void adjust_screen_tap_event_handler(lv_event_t *e);

AdjustState::AdjustState() : 
    adjustDuration(0), 
    lastActivity(0),
    titleLabel(nullptr),
    durationLabel(nullptr),
    instructionLabel(nullptr)
{}

void AdjustState::processScreenTap() {
    Serial.println("Adjust State: Screen tapped - Saving duration");
    StateMachine::idleState.setTimer(this->adjustDuration);
    // Potentially show a brief confirmation animation/message via DisplayController if desired in future
    stateMachine.changeState(&StateMachine::idleState);
}

static void adjust_screen_tap_event_handler(lv_event_t *e) {
    AdjustState* self = (AdjustState*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED && self) {
        self->processScreenTap();
    }
}

void AdjustState::enter()
{
  Serial.println("Entering Adjust State (Testing Label Validity)");

  adjustDuration = StateMachine::idleState.getDefaultDuration();
  Serial.printf("Adjust State: Starting duration = %d\n", adjustDuration);

  lastActivity = millis();
  ledController.setSolid(FD_COLOR_AMBER);

  lv_obj_t *screen = lv_screen_active();
  if (!screen) {
    Serial.println("AdjustState::enter() - FATAL: lv_screen_active() returned NULL!");
    return;
  }
  Serial.printf("AdjustState: Screen ptr in enter(): %p\n", (void*)screen);
  lv_obj_clean(screen); 
  Serial.println("AdjustState: lv_obj_clean(screen) called.");

  // Create Title Label
  titleLabel = lv_label_create(screen);
  Serial.printf("AdjustState: titleLabel pointer after creation: %p\n", (void*)titleLabel);
  if (titleLabel) {
    lv_label_set_text(titleLabel, "Adjust Duration");
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 20);
    Serial.println("AdjustState: titleLabel created and styled.");
  } else {
    Serial.println("AdjustState: titleLabel creation FAILED (is NULL)!");
  }

  // Create Duration Label
  durationLabel = lv_label_create(screen);
  Serial.printf("AdjustState: durationLabel pointer after creation: %p\n", (void*)durationLabel);
  if (durationLabel) { 
    lv_obj_set_style_text_font(durationLabel, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(durationLabel, "%d min", adjustDuration);
    lv_obj_align(durationLabel, LV_ALIGN_CENTER, 0, -10);
    Serial.println("AdjustState: durationLabel created and styled.");
  } else {
    Serial.println("AdjustState: durationLabel creation FAILED (is NULL)!");
  }
  
  Serial.printf("AdjustState: After creating both labels, screen child count: %u\n", lv_obj_get_child_cnt(screen));
  Serial.printf("AdjustState: titleLabel valid?: %d, durationLabel valid?: %d\n", 
                titleLabel ? lv_obj_is_valid(titleLabel) : 0, 
                durationLabel ? lv_obj_is_valid(durationLabel) : 0);

  // instructionLabel = nullptr; // Not creating instruction label in this test

  // Encoder Handler
  inputController.onEncoderRotateHandler([this, screen](int delta) { 
      Serial.printf("Adjust State: Encoder delta: %d. Current adjustDuration: %d\n", delta, this->adjustDuration);
      
      this->adjustDuration += delta; 
      
      if (this->adjustDuration < MIN_TIMER) {
          this->adjustDuration = MIN_TIMER;
      } else if (this->adjustDuration > MAX_TIMER) {
          this->adjustDuration = MAX_TIMER;
      }
      Serial.printf("AdjustState: New adjustDuration: %d\n", this->adjustDuration);
      
      Serial.printf("AdjustState: Encoder Handler - Screen ptr: %p. Child count: %u\n", (void*)screen, lv_obj_get_child_cnt(screen));
      Serial.printf("AdjustState: Encoder Handler - titleLabel ptr: %p, valid?: %d\n", (void*)this->titleLabel, this->titleLabel ? lv_obj_is_valid(this->titleLabel) : 0);
      Serial.printf("AdjustState: Encoder Handler - durationLabel ptr: %p, valid?: %d\n", (void*)this->durationLabel, this->durationLabel ? lv_obj_is_valid(this->durationLabel) : 0);

      if (this->durationLabel && lv_obj_is_valid(this->durationLabel)) {
          lv_label_set_text_fmt(this->durationLabel, "%d min", this->adjustDuration);
          Serial.println("AdjustState: durationLabel text updated.");
      } else {
          Serial.println("AdjustState: durationLabel is NULL or INVALID in encoder handler before set_text_fmt!");
      }
      this->lastActivity = millis(); 
  });

  // Screen Tap Handler
  lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(screen, adjust_screen_tap_event_handler, LV_EVENT_CLICKED, this);
  Serial.println("AdjustState: Screen tap event added.");

  lv_refr_now(NULL);
  Serial.println("AdjustState: enter() finished.");
}

void AdjustState::update()
{
  inputController.update(); 
  if (millis() - lastActivity >= (CHANGE_TIMEOUT * 1000))
  {
    Serial.println("AdjustState: Timeout - Reverting to Idle without saving.");
    stateMachine.changeState(&StateMachine::idleState);
  }
}

void AdjustState::exit()
{
  Serial.println("Exiting Adjust State");
  inputController.releaseHandlers();

  lv_obj_t *screen = lv_screen_active();
  if (screen) {
      lv_obj_remove_event_cb_with_user_data(screen, adjust_screen_tap_event_handler, this);
      lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE);
      Serial.println("AdjustState: Screen tap event removed.");
  }

  // Delete LVGL objects
  if (titleLabel) { lv_obj_del(titleLabel); titleLabel = nullptr; }
  if (durationLabel) { lv_obj_del(durationLabel); durationLabel = nullptr; }
  // if (instructionLabel) { lv_obj_del(instructionLabel); instructionLabel = nullptr; } // Still not created
  
  ledController.turnOff(); 
  Serial.println("AdjustState: exit() finished.");
}

// This method might be called by other states to preset the duration if needed.
void AdjustState::adjustTimer(int duration)
{
  adjustDuration = duration;
  if (durationLabel && lv_obj_is_valid(durationLabel)) { // Check if label exists and is valid
    lv_label_set_text_fmt(durationLabel, "%d min", adjustDuration);
  }
}
