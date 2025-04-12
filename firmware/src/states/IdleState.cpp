#include "StateMachine.h"
#include "Controllers.h"

IdleState::IdleState() : defaultDuration(0), lastActivity(0)
{

  if (nvs_flash_init() != ESP_OK)
  {
    Serial.println("NVS Flash Init Failed");
  }
  else
  {
    Serial.println("NVS initialized successfully.");
  }

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
  ledController.setBreath(BLUE, -1, false, 5);

  // Register state-specific handlers
  inputController.onPressHandler([this]()
                                 {
                                   Serial.println("Idle State: Button pressed - Go to Project Select");
                                   // Store the current default duration for TimerState later
                                   stateMachine.setPendingDuration(this->defaultDuration);
                                   // stateMachine.adjustState.adjustTimer(this->defaultDuration); // No longer needed
                                   // stateMachine.changeState(&StateMachine::adjustState); // Go to Project Select instead
                                   stateMachine.changeState(&StateMachine::projectSelectState); });

  inputController.onLongPressHandler([this]()
                                     {
                                       Serial.println("Idle State: Button long pressed");
                                       stateMachine.changeState(&StateMachine::resetState); // Transition to Reset State
                                     });

  inputController.onEncoderRotateHandler([this](int delta)
                                         {
                                           Serial.println("Idle State: Encoder turned - Go to Adjust Duration");
                                           // StateMachine::adjustState.adjustTimer(this->defaultDuration); // AdjustState will fetch its own starting point
                                           stateMachine.changeState(&StateMachine::adjustState); // Transition to Adjust State
                                         });

  lastActivity = millis(); // Activity timer
}

void IdleState::update()
{
  static unsigned long lastUpdateTime = 0;

  // Controllers updates
  inputController.update();
  ledController.update();
  networkController.update();

  // Restore unconditional redraw
  displayController.drawIdleScreen(defaultDuration, networkController.isWiFiConnected());

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
}

void IdleState::setTimer(int duration)
{
  defaultDuration = duration;

  preferences.begin("focusdial", true);
  preferences.putInt("timer", defaultDuration);
  preferences.end();
}

int IdleState::getDefaultDuration() const
{
  return defaultDuration;
}