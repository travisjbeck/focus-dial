#include "StateMachine.h"
#include "Controllers.h"

// Remove local storage for name/color, only need LED color
TimerState::TimerState() : duration(0), elapsedTime(0), startTime(0), currentLedColor(0) {}

void TimerState::enter()
{
  Serial.println("Entering Timer State");
  startTime = millis() - (elapsedTime * 1000); // Adjust start time based on potentially elapsed time

  // On initial entry (not resume), fetch project color based on stored ID
  if (elapsedTime == 0)
  {
    Serial.println("Timer State: Initial entry");
    String pendingId = stateMachine.getPendingProjectId();
    String projectColorHex = "#FFFFFF"; // Default to White

    if (!pendingId.isEmpty())
    {
      // Find project by ID
      const auto &allProjects = getProjectManagerInstance().getProjects();
      bool found = false;
      for (const auto &p : allProjects)
      {
        if (p.device_project_id == pendingId)
        {
          projectColorHex = p.color;
          Serial.printf("Found project for timer: ID=%s, Color=%s\n", pendingId.c_str(), projectColorHex.c_str());
          found = true;
          break;
        }
      }
      if (!found)
      {
        Serial.printf("Warning: Could not find project color for pending ID: %s. Using default white.\n", pendingId.c_str());
      }
    }
    else
    {
      Serial.println("Timer started with no project selected.");
    }

    currentLedColor = LEDController::hexColorToUint32(projectColorHex);
    // StateMachine no longer stores name/color, clear only ID
    // stateMachine.clearPendingProject(); // ID cleared implicitly when used by webhook
  }
  else
  {
    Serial.printf("Timer State: Resuming with stored color %06X\n", currentLedColor);
  }

  // Start LED Fill and Decay Animation using stored color and REMAINING duration
  uint32_t remainingDurationMs = (this->duration * 60 - this->elapsedTime) * 1000;
  if (remainingDurationMs > 0)
  {
    ledController.startFillAndDecay(currentLedColor, remainingDurationMs);
  }
  else
  {
    ledController.turnOff(); // Turn off if timer already expired on resume
  }

  // Setup Input Handlers
  inputController.onPressHandler([this]()
                                 {
                                   Serial.println("Timer State: Button Pressed - Pausing");
                                   // Send 'stop' action, webhook handler uses stored ID
                                   networkController.sendWebhookAction("stop"); 
                                   displayController.showTimerPause();
                                   StateMachine::pausedState.setPause(this->duration, this->elapsedTime);
                                   stateMachine.changeState(&StateMachine::pausedState); });

  inputController.onDoublePressHandler([this]()
                                       {
                                         Serial.println("Timer State: Button Double Pressed - Canceling");
                                         // Send 'stop' action, webhook handler uses stored ID
                                         networkController.sendWebhookAction("stop");
                                         displayController.showCancel();
                                         stateMachine.changeState(&StateMachine::idleState); });

  // Send 'start' action ONLY on initial entry
  if (elapsedTime == 0)
  {
    networkController.sendWebhookAction("start");
  }
}

void TimerState::update()
{
  inputController.update();
  ledController.update();

  unsigned long currentTime = millis();
  elapsedTime = (currentTime - startTime) / 1000;

  int remainingSeconds = duration * 60 - elapsedTime;

  displayController.drawTimerScreen(remainingSeconds);

  // Check if the timer is done
  if (remainingSeconds <= 0)
  {
    Serial.println("Timer State: Done");
    displayController.showTimerDone();
    stateMachine.changeState(&StateMachine::doneState); // Transition to Done State
  }
}

void TimerState::exit()
{
  inputController.releaseHandlers();
  // Don't reset local state (like currentLedColor) as it's needed if resuming from pause
  Serial.println("Exiting Timer State");
}

void TimerState::setTimer(int duration, unsigned long elapsedTime)
{
  this->duration = duration;
  this->elapsedTime = elapsedTime;
}