#include "StateMachine.h"
#include "Controllers.h"

TimerState::TimerState() : duration(0), elapsedTime(0), startTime(0), currentLedColor(0), currentProjectName(""), currentProjectColorHex("") {}

void TimerState::enter()
{
  Serial.println("Entering Timer State");
  startTime = millis() - (elapsedTime * 1000); // Adjust start time based on potentially elapsed time

  String projectName = ""; // Initialize project name for webhook use

  // Retrieve project color ONLY on initial entry (elapsedTime == 0)
  if (elapsedTime == 0)
  {
    Serial.println("Timer State: Initial entry");
    currentProjectName = stateMachine.getPendingProjectName();      // Store name
    currentProjectColorHex = stateMachine.getPendingProjectColor(); // Store hex string
    bool hasProject = !currentProjectName.isEmpty();

    Serial.printf("Timer for project: '%s', Color: %s\n", currentProjectName.c_str(), currentProjectColorHex.c_str());

    if (hasProject && currentProjectColorHex.length() > 0)
    {
      currentLedColor = LEDController::hexColorToUint32(currentProjectColorHex);
    }
    else
    {
      currentLedColor = LEDController::hexColorToUint32("#FFFFFF"); // Default to White
      currentProjectColorHex = "#FFFFFF";                           // Ensure hex string is white too
    }
    stateMachine.clearPendingProject();
  }
  else
  {
    Serial.printf("Timer State: Resuming with stored color %06X\n", currentLedColor);
    // Note: currentProjectName and currentProjectColorHex retain their values from initial entry
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

  bool finalHasProject = !currentProjectName.isEmpty();

  // Setup Input Handlers (capture necessary info)
  inputController.onPressHandler([this]() // Remove captures, use member vars
                                 {
                                   Serial.println("Timer State: Button Pressed - Pausing");
                                   String action = "stop";
                                   if (!currentProjectName.isEmpty())
                                   {
                                     action += "|" + currentProjectName + "|" + currentProjectColorHex;
                                   }
                                   networkController.sendWebhookAction(action);
                                   displayController.showTimerPause();
                                   StateMachine::pausedState.setPause(this->duration, this->elapsedTime);
                                   stateMachine.changeState(&StateMachine::pausedState); });

  inputController.onDoublePressHandler([this]() // Remove captures, use member vars
                                       {
                                         Serial.println("Timer State: Button Double Pressed - Canceling");
                                         String action = "stop";
                                         if (!currentProjectName.isEmpty())
                                         {
                                           action += "|" + currentProjectName + "|" + currentProjectColorHex;
                                         }
                                         networkController.sendWebhookAction(action);
                                         displayController.showCancel();
                                         stateMachine.changeState(&StateMachine::idleState); });

  // Send 'Start' webhook ONLY on initial entry
  if (elapsedTime == 0)
  {
    String startAction = "start";
    if (finalHasProject) // Use finalHasProject for consistency
    {
      startAction += "|" + currentProjectName + "|" + currentProjectColorHex;
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
  // ledController.turnOff(); // Don't turn off LEDs here, PausedState might need them?
  // Reset potentially large string members
  currentProjectName = "";
  currentProjectColorHex = "";
  Serial.println("Exiting Timer State");
}

void TimerState::setTimer(int duration, unsigned long elapsedTime)
{
  this->duration = duration;
  this->elapsedTime = elapsedTime;
}