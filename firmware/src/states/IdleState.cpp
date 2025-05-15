#include "StateMachine.h"
#include "Controllers.h"
#include <lvgl.h> // Added for LVGL events

// Static event callback for screen click
static void idle_screen_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // We can use the global stateMachine instance as IdleState is tightly coupled with it
    // and projectSelectState is a static member of StateMachine.

    if (code == LV_EVENT_CLICKED) {
        Serial.println("IdleState: Screen clicked - Go to Project Select");
        // Access defaultDuration from the static idleState instance within StateMachine
        stateMachine.setPendingDuration(StateMachine::idleState.getDefaultDuration()); 
        stateMachine.changeState(&StateMachine::projectSelectState);
    }
}

IdleState::IdleState() : defaultDuration(0), lastActivity(0)
{

  // Load the default duration
  if (preferences.begin("focusdial", true))
  {
    defaultDuration = preferences.getInt("timer", DEFAULT_TIMER);
    preferences.end();
  }
}

void IdleState::enter()
{
  Serial.println("Entering Idle State");
  ledController.setBreath(FD_BLUE, -1, false, 5);

  // Register state-specific handlers
  // inputController.onPressHandler([this]() // Button deprecated - Phase 1
  //                                {
  //                                  Serial.println("Idle State: Button pressed - Go to Project Select");
  //                                  // Store the current default duration for TimerState later
  //                                  stateMachine.setPendingDuration(this->defaultDuration);
  //                                  // stateMachine.adjustState.adjustTimer(this->defaultDuration); // No longer needed
  //                                  // stateMachine.changeState(&StateMachine::adjustState); // Go to Project Select instead
  //                                  stateMachine.changeState(&StateMachine::projectSelectState); });

  // inputController.onLongPressHandler([this]() // Button deprecated - Phase 1
  //                                    {
  //                                      Serial.println("Idle State: Button long pressed");
  //                                      stateMachine.changeState(&StateMachine::resetState); // Transition to Reset State
  //                                    });

  inputController.onEncoderRotateHandler([this](int delta)
                                         {
                                           Serial.println("Idle State: Encoder turned - Go to Adjust Duration");
                                           // StateMachine::adjustState.adjustTimer(this->defaultDuration); // AdjustState will fetch its own starting point
                                           stateMachine.changeState(&StateMachine::adjustState); // Transition to Adjust State
                                         });

  // Add LVGL screen click event
  lv_obj_t *screen = lv_screen_active();
  if (screen) { // Important to check if screen is not NULL
      lv_obj_add_event_cb(screen, idle_screen_event_cb, LV_EVENT_CLICKED, nullptr); 
      // Make sure the screen is clickable - important!
      lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
      Serial.println("IdleState: LVGL screen click event added.");
  } else {
      Serial.println("IdleState::enter() - Error: lv_screen_active() returned NULL when adding event!");
  }

  lastActivity = millis(); // Activity timer
}

void IdleState::update()
{
  static unsigned long lastUpdateTime = 0;

  // Controllers updates
  inputController.update();
  ledController.update();
  networkController.update();

  // Restore unconditional redraw ONLY if IdleState is the current state
  if (stateMachine.getCurrentState() == this) {
    displayController.drawIdleScreen(defaultDuration, networkController.isWiFiConnected());
  }

  // Check if sleep timeout is reached
  if (millis() - lastActivity >= (SLEEP_TIMOUT * 60 * 1000))
  {
    Serial.println("Idle State: Activity timeout");
    stateMachine.changeState(&StateMachine::sleepState); // Transition to Sleep State
  }
}

void IdleState::exit()
{
  Serial.println("Exiting Idle State");
  inputController.releaseHandlers();
  ledController.turnOff();

  // Remove LVGL screen click event
  lv_obj_t *screen = lv_screen_active();
  if (screen) { // Check if screen is not NULL
      lv_obj_remove_event_cb(screen, idle_screen_event_cb);
      // Optionally remove clickable flag if it causes issues, though usually not necessary
      // lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE);
      Serial.println("IdleState: LVGL screen click event removed.");
  } else {
      Serial.println("IdleState::exit() - Warning: lv_screen_active() returned NULL when removing event!");
  }
}

void IdleState::setTimer(int duration)
{
  defaultDuration = duration;

  if (preferences.begin("focusdial", false))
  {
    preferences.putInt("timer", defaultDuration);
    preferences.end();
  }
  else
  {
    Serial.println("IdleState: Failed to open NVS for writing timer duration.");
  }
}

int IdleState::getDefaultDuration() const
{
  return defaultDuration;
}

void IdleState::restoreDefaultLEDPattern()
{
  Serial.println("IdleState: Restoring default LED pattern");
  // Set the LEDs back to the regular idle state animation
  ledController.setBreath(FD_BLUE, -1, false, 5);
}