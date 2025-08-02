#include "TouchManager.h"
#include "../state_machine/include/StateMachine.h"
#include "ScreenManager.h"
#include "../state_machine/states/ProjectSelectState.h"
#include "../audio/AlarmController.h"
#include <esp_log.h>
#include <lvgl.h>

TouchManager::TouchManager() :
    touchState(IDLE),
    touchStartTime(0),
    touchStartX(0),
    touchStartY(0),
    lastTouchX(0),
    lastTouchY(0),
    longPressThresholdMs(1500),  // 1.5 seconds for long press
    stateMachine(nullptr),
    screenManager(nullptr)
{
}

TouchManager::~TouchManager() {
}

void TouchManager::init(StateMachine* sm, ScreenManager* scm) {
    stateMachine = sm;
    screenManager = scm;
    touchState = IDLE;
    ESP_LOGI(getLogTag(), "TouchManager initialized - release-only touch system active");
}

void TouchManager::processTouch(bool touchDetected, int16_t x, int16_t y) {
    unsigned long currentTime = millis();
    
    if (touchDetected) {
        // Touch detected - update activity time
        if (stateMachine) {
            stateMachine->updateActivityTime();
        }
        
        if (touchState == IDLE) {
            // Start new touch sequence
            touchState = PRESSING;
            touchStartTime = currentTime;
            touchStartX = x;
            touchStartY = y;
            lastTouchX = x;
            lastTouchY = y;
            
            logTouchEvent("PRESS_START");
            ESP_LOGD(getLogTag(), "Touch started at (%d, %d)", x, y);
        } else if (touchState == PRESSING) {
            // Update current touch position
            lastTouchX = x;
            lastTouchY = y;
            
            // Check if we've reached long press threshold
            unsigned long pressDuration = currentTime - touchStartTime;
            if (pressDuration >= longPressThresholdMs) {
                // Trigger long press immediately
                touchState = LONG_TRIGGERED;
                logTouchEvent("LONG_PRESS_TRIGGERED", pressDuration);
                handleLongPressImmediate();
            }
        }
        // During LONG_TRIGGERED or BLOCKED: do nothing
        
    } else {
        // Touch released
        if (touchState == PRESSING) {
            // Check for swipe gesture first (on project selection and timer screens)
            const char* screenName = getCurrentScreenName();
            if (strcmp(screenName, "ProjectSelectState") == 0 || strcmp(screenName, "TimerState") == 0) {
                if (detectSwipeGesture(lastTouchX, lastTouchY)) {
                    // Swipe detected, don't process as normal touch
                    touchState = IDLE;
                    return;
                }
            }
            
            // Valid release before long press threshold - process as short press
            unsigned long pressDuration = currentTime - touchStartTime;
            logTouchEvent("RELEASE", pressDuration);
            
            touchState = IDLE;
            handleTouchRelease(pressDuration);
            
        } else if (touchState == LONG_TRIGGERED) {
            // Release after long press was already triggered - just reset state
            logTouchEvent("RELEASE_AFTER_LONG");
            touchState = IDLE;
            
        } else if (touchState == BLOCKED) {
            // Release during blocked state - ignore completely
            logTouchEvent("RELEASE_BLOCKED");
            touchState = IDLE;
            
        }
        // If IDLE: spurious release, ignore
    }
}

void TouchManager::blockTouchDuringTransition() {
    if (touchState == PRESSING || touchState == LONG_TRIGGERED) {
        ESP_LOGI(getLogTag(), "Blocking touch during state transition");
        touchState = BLOCKED;
    }
}

void TouchManager::unblockTouchAfterTransition() {
    if (touchState == BLOCKED) {
        ESP_LOGI(getLogTag(), "Unblocking touch after state transition");
        touchState = IDLE;  // Reset to idle, ignore any ongoing touch
    }
}

void TouchManager::handleTouchRelease(unsigned long pressDuration) {
    bool isLongPress = (pressDuration >= longPressThresholdMs);
    
    ESP_LOGI(getLogTag(), "Processing %s press (duration: %lums)", 
             isLongPress ? "LONG" : "SHORT", pressDuration);
    
    if (isLongPress) {
        handleLongPress();
    } else {
        handleShortPress();
    }
}

void TouchManager::handleShortPress() {
    // Check if confirmation dialog is visible first
    if (screenManager && screenManager->isConfirmDialogVisible()) {
        ESP_LOGI(getLogTag(), "Short press detected with confirmation dialog visible");
        handleConfirmDialogTouch();
        return;
    }
    
    const char* currentScreen = getCurrentScreenName();
    ESP_LOGI(getLogTag(), "Short press on %s screen", currentScreen);
    
    if (strcmp(currentScreen, "IdleState") == 0) {
        handleIdleScreenTouch();
    } else if (strcmp(currentScreen, "AdjustState") == 0) {
        handleAdjustScreenTouch();
    } else if (strcmp(currentScreen, "ProjectSelectState") == 0) {
        handleProjectScreenTouch();
    } else if (strcmp(currentScreen, "TimerState") == 0) {
        handleTimerScreenTouch(false);
    } else if (strcmp(currentScreen, "PausedState") == 0) {
        handlePausedScreenTouch(false);
    } else if (strcmp(currentScreen, "DoneState") == 0) {
        handleDoneScreenTouch();
    }
}

void TouchManager::handleLongPress() {
    const char* currentScreen = getCurrentScreenName();
    ESP_LOGI(getLogTag(), "Long press on %s screen", currentScreen);
    
    // Long press only available in Paused state (TimerState now uses swipe gesture)
    if (strcmp(currentScreen, "PausedState") == 0) {
        handlePausedScreenTouch(true);
    } else {
        ESP_LOGD(getLogTag(), "Long press ignored on %s screen", currentScreen);
    }
}

void TouchManager::handleLongPressImmediate() {
    const char* currentScreen = getCurrentScreenName();
    ESP_LOGI(getLogTag(), "Long press IMMEDIATE on %s screen", currentScreen);
    
    // Long press only available in Timer and Paused states
    if (strcmp(currentScreen, "TimerState") == 0) {
        handleTimerScreenTouch(true);
    } else if (strcmp(currentScreen, "PausedState") == 0) {
        handlePausedScreenTouch(true);
    } else {
        ESP_LOGD(getLogTag(), "Long press ignored on %s screen", currentScreen);
    }
}

void TouchManager::handleIdleScreenTouch() {
    ESP_LOGI(getLogTag(), ">>> TRANSITION: IdleState -> ProjectSelectState <<<");
    blockTouchDuringTransition();
    stateMachine->changeState((State*)stateMachine->projectSelectState);
}

void TouchManager::handleAdjustScreenTouch() {
    ESP_LOGI(getLogTag(), ">>> TRANSITION: AdjustState -> IdleState <<<");
    blockTouchDuringTransition();
    stateMachine->changeState((State*)stateMachine->idleState);
}

void TouchManager::handleProjectScreenTouch() {
    // Reset activity timer on any touch
    ProjectSelectState* projectState = (ProjectSelectState*)stateMachine->projectSelectState;
    if (projectState) {
        projectState->resetActivityTimer();
    }
    
    // Normal tap behavior - select project and go to timer
    ESP_LOGI(getLogTag(), ">>> TRANSITION: ProjectSelectState -> TimerState <<<");
    blockTouchDuringTransition();
    stateMachine->changeState((State*)stateMachine->timerState);
}

void TouchManager::handleTimerScreenTouch(bool isLongPress) {
    if (isLongPress) {
        ESP_LOGI(getLogTag(), ">>> LONG PRESS: Showing confirmation dialog <<<");
        screenManager->showConfirmDialog("End Timer?");
    } else {
        ESP_LOGI(getLogTag(), ">>> TRANSITION: TimerState -> PausedState <<<");
        blockTouchDuringTransition();
        stateMachine->changeState((State*)stateMachine->pausedState);
    }
}

void TouchManager::handlePausedScreenTouch(bool isLongPress) {
    if (isLongPress) {
        ESP_LOGI(getLogTag(), ">>> LONG PRESS: Showing confirmation dialog <<<");
        screenManager->showConfirmDialog("End Timer?");
    } else {
        ESP_LOGI(getLogTag(), ">>> TRANSITION: PausedState -> TimerState (Resume) <<<");
        blockTouchDuringTransition();
        stateMachine->changeState((State*)stateMachine->timerState);
    }
}

void TouchManager::handleConfirmDialogTouch() {
    // Get the current selection from the dialog
    bool confirmSelection = screenManager->getConfirmSelection();
    ESP_LOGI(getLogTag(), "Confirmation dialog touch - selection: %s, touchState: %d", 
             confirmSelection ? "YES" : "NO", (int)touchState);
    
    if (confirmSelection) {
        // YES selected - end timer and go to idle
        ESP_LOGI(getLogTag(), ">>> CONFIRMED: Ending timer <<<");
        screenManager->hideConfirmDialog();
        blockTouchDuringTransition();
        stateMachine->changeState((State*)stateMachine->idleState);
    } else {
        // NO selected - cancel and hide dialog
        ESP_LOGI(getLogTag(), ">>> CANCELLED: Hiding dialog <<<");
        screenManager->hideConfirmDialog();
    }
}

void TouchManager::handleDoneScreenTouch() {
    ESP_LOGI(getLogTag(), ">>> TRANSITION: DoneState -> IdleState (tap to continue) <<<");
    
    // Stop alarm if playing
    extern AlarmController alarmController;
    if (alarmController.isPlaying()) {
        ESP_LOGI(getLogTag(), "Stopping alarm on user interaction");
        alarmController.stopAlarm();
    }
    
    blockTouchDuringTransition();
    stateMachine->changeState((State*)stateMachine->idleState);
}

const char* TouchManager::getCurrentScreenName() const {
    if (!stateMachine) return "Unknown";
    
    State* currentState = stateMachine->getCurrentState();
    if (!currentState) return "Unknown";
    
    return currentState->getStateName();
}

void TouchManager::logTouchEvent(const char* event, unsigned long duration) const {
    if (duration > 0) {
        ESP_LOGD(getLogTag(), "[TOUCH] %s (duration: %lums)", event, duration);
    } else {
        ESP_LOGD(getLogTag(), "[TOUCH] %s", event);
    }
}

bool TouchManager::detectSwipeGesture(int16_t currentX, int16_t currentY) {
    // Calculate movement from start position
    int16_t deltaX = currentX - touchStartX;
    int16_t deltaY = currentY - touchStartY;
    
    ESP_LOGD(getLogTag(), "Swipe detection: start(%d,%d) -> end(%d,%d), delta(%d,%d)", 
             touchStartX, touchStartY, currentX, currentY, deltaX, deltaY);
    
    // Check for left swipe (negative X movement)
    // Very forgiving: just need significant leftward movement with reasonable vertical tolerance
    if (deltaX < -MIN_SWIPE_DISTANCE && abs(deltaY) < MAX_SWIPE_Y_VARIANCE) {
        const char* currentScreen = getCurrentScreenName();
        ESP_LOGI(getLogTag(), ">>> LEFT SWIPE DETECTED on %s <<<", currentScreen);
        ESP_LOGI(getLogTag(), "Swipe details: deltaX=%d, deltaY=%d (threshold: %d)", 
                 deltaX, deltaY, MIN_SWIPE_DISTANCE);
        
        if (strcmp(currentScreen, "ProjectSelectState") == 0) {
            // Project selection screen: go back to idle
            ProjectSelectState* projectState = (ProjectSelectState*)stateMachine->projectSelectState;
            if (projectState) {
                projectState->resetActivityTimer();
            }
            blockTouchDuringTransition();
            stateMachine->changeState((State*)stateMachine->idleState);
            
        } else if (strcmp(currentScreen, "TimerState") == 0) {
            // Timer screen: show cancel confirmation dialog
            ESP_LOGI(getLogTag(), ">>> SWIPE: Showing timer cancel confirmation <<<");
            screenManager->showConfirmDialog("End Timer?");
        }
        
        return true;
    }
    
    return false;
}