#include "StateMachine.h"
#include "Controllers.h"

TimerState::TimerState() : duration(0), elapsedTime(0), startTime(0), currentLedColor(0) {}

void TimerState::enter()
{
  Serial.println("Entering Timer State");
  startTime = millis() - (elapsedTime * 1000); // Adjust start time based on potentially elapsed time

  String projectName = ""; // Initialize project name for webhook use

  // Retrieve project color ONLY on initial entry (elapsedTime == 0)
  if (elapsedTime == 0)
  {
    Serial.println("Timer State: Initial entry");
    projectName = stateMachine.getPendingProjectName();
    String projectColorStr = stateMachine.getPendingProjectColor();
    bool hasProject = !projectName.isEmpty();

    Serial.printf("Timer for project: '%s', Color: %s\n", projectName.c_str(), projectColorStr.c_str());

    if (hasProject && projectColorStr.length() > 0)
    {
      currentLedColor = LEDController::hexColorToUint32(projectColorStr);
    }
    else
    {
      currentLedColor = LEDController::hexColorToUint32("#FFFFFF"); // Default to White
    }
    // Clear pending project info only after initial retrieval
    stateMachine.clearPendingProject();
  }
  else
  {
    Serial.printf("Timer State: Resuming with stored color %06X\n", currentLedColor);
    // If resuming, we need the project name again for webhooks if applicable
    // This assumes project context doesn't change mid-timer. Fetch from a more persistent source if needed.
    // For now, let's assume projectName isn't strictly needed on resume for webhook
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

  bool finalHasProject = !projectName.isEmpty(); // Use potentially updated projectName if fetched on resume

  // Setup Input Handlers
  inputController.onPressHandler([this, finalHasProject, projectName]() // Capture necessary info
                                 {
                                   Serial.println("Timer State: Button Pressed - Pausing");
                                   // Send 'Stop' webhook (pause)
                                   String action = "stop";
                                   if (finalHasProject)
                                   {
                                     action += "|" + projectName; // Append project name
                                   }
                                   networkController.sendWebhookAction(action);
                                   displayController.showTimerPause();
                                   // Transition to PausedState and set elapsed time
                                   StateMachine::pausedState.setPause(this->duration, this->elapsedTime);
                                   stateMachine.changeState(&StateMachine::pausedState); });

  inputController.onDoublePressHandler([this, finalHasProject, projectName]()
                                       {
                                         Serial.println("Timer State: Button Double Pressed - Canceling");
                                         // Send 'Stop' webhook (canceled)
                                         String action = "stop";
                                         if (finalHasProject)
                                         {
                                           action += "|" + projectName; // Append project name
                                         }
                                         networkController.sendWebhookAction(action);
                                         displayController.showCancel();
                                         stateMachine.changeState(&StateMachine::idleState); });

  // Send 'Start' webhook ONLY on initial entry
  if (elapsedTime == 0)
  {
    String startAction = "start";
    if (finalHasProject)
    {
      startAction += "|" + projectName; // Append project name
    }
    networkController.sendWebhookAction(startAction);
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
  // networkController.stopBluetooth(); // Let NetworkController manage BT connection
  ledController.turnOff();
  Serial.println("Exiting Timer State");
}

void TimerState::setTimer(int duration, unsigned long elapsedTime)
{
  this->duration = duration;
  this->elapsedTime = elapsedTime;
}