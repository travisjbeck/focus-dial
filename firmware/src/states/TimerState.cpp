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

  // Handle LED state based on duration
  if (this->duration == 0) // Indeterminate mode
  {
    Serial.println("Timer State: Indeterminate mode - starting Radar Sweep LED animation.");
    ledController.startRadarSweep(currentLedColor); // Start new sweep animation
  }
  else // Countdown mode
  {
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
  }

  // Setup Input Handlers
  inputController.onPressHandler([this]()
                                 {
                                   // Send 'stop' action first (applies to both modes)
                                   networkController.sendWebhookAction("stop", this->duration, this->elapsedTime);

                                   if (this->duration == 0) // Indeterminate mode
                                   {
                                     Serial.println("Timer State: Button Pressed - Stopping Indeterminate Timer");
                                     // Pass final elapsed time to DoneState via StateMachine
                                     stateMachine.setPendingElapsedTime(this->elapsedTime);
                                     displayController.showTimerDone(); // Show done animation
                                     stateMachine.changeState(&StateMachine::doneState);
                                   }
                                   else // Countdown mode
                                   {
                                     Serial.println("Timer State: Button Pressed - Pausing Countdown Timer");
                                     displayController.showTimerPause();
                                     // Pass current duration and elapsed time to PausedState
                                     StateMachine::pausedState.setPause(this->duration, this->elapsedTime);
                                     stateMachine.changeState(&StateMachine::pausedState);
                                   } });

  inputController.onDoublePressHandler([this]()
                                       {
                                         Serial.println("Timer State: Button Double Pressed - Canceling");
                                         // Send 'stop' action with current elapsed time
                                         networkController.sendWebhookAction("stop", this->duration, this->elapsedTime);
                                         displayController.showCancel();
                                         stateMachine.changeState(&StateMachine::idleState); });

  // Send 'start' action ONLY on initial entry
  if (elapsedTime == 0)
  {
    networkController.sendWebhookAction("start", this->duration, 0); // Pass set duration, 0 elapsed
  }
}

void TimerState::update()
{
  inputController.update();
  ledController.update(); // This now handles RadarSweep update automatically

  unsigned long currentTime = millis();
  elapsedTime = (currentTime - startTime) / 1000;

  if (duration == 0) // Indeterminate Mode
  {
    // Display elapsed time counting up
    displayController.drawTimerScreen(elapsedTime, true);
    // No automatic completion check, relies on button press
  }
  else // Countdown Mode
  {
    int remainingSeconds = duration * 60 - elapsedTime;

    displayController.drawTimerScreen(remainingSeconds, false);

    // Check if the timer is done
    if (remainingSeconds <= 0)
    {
      Serial.println("Timer State: Done (Countdown)");
      // Pass final elapsed time (which is duration * 60) to DoneState via StateMachine
      stateMachine.setPendingElapsedTime(this->duration * 60);
      displayController.showTimerDone();
      stateMachine.changeState(&StateMachine::doneState); // Transition to Done State
    }
  }
}

void TimerState::exit()
{
  inputController.releaseHandlers();
  // ledController.stopCurrentAnimation(); // Ensure animation stops - Likely handled by next state's LED call
  Serial.println("Exiting Timer State");
}

void TimerState::setTimer(int duration, unsigned long elapsedTime)
{
  this->duration = duration;
  this->elapsedTime = elapsedTime;
}