#include "UIEventHandler.h"
#include "ScreenManager.h"
#include "../state_machine/include/StateMachine.h"
#include "../ProjectManager.h"
#include <Arduino.h>
#include "HWCDC.h"
#include <lvgl.h>

// USB Serial for ESP32-S3
extern HWCDC USBSerial;

// Static members
ScreenManager* UIEventHandler::screenManager = nullptr;
StateMachine* UIEventHandler::stateMachine = nullptr;

// External StateMachine instance
extern StateMachine stateMachine;

// External screen transition time
extern unsigned long lastScreenTransitionTime;

// External touch sequence tracking
extern bool touchSequenceActive;
extern unsigned long touchSequenceStartTime;

// Track if we've processed a click in this touch sequence
static bool clickProcessedInSequence = false;
static unsigned long lastClickSequenceTime = 0;

// Manual long press detection
static unsigned long touchStartTime = 0;
static bool longPressDetected = false;
static const unsigned long MANUAL_LONG_PRESS_MS = 1500; // 1.5 seconds

void UIEventHandler::init(ScreenManager* screenMgr, StateMachine* stateMachine) {
    UIEventHandler::screenManager = screenMgr;
    UIEventHandler::stateMachine = stateMachine;
}

void UIEventHandler::handleIdleScreenTap(lv_event_t* e) {
    if (!stateMachine) return;
    
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    
    // Ignore phantom touches after screen transitions
    unsigned long timeSinceTransition = millis() - lastScreenTransitionTime;
    if (timeSinceTransition < 200) {
        USBSerial.print("[IDLE] Ignoring phantom touch, only ");
        USBSerial.print(timeSinceTransition);
        USBSerial.println("ms since screen transition");
        return;
    }
    
    USBSerial.print("[IDLE] Event target: 0x");
    USBSerial.print((uint32_t)target, HEX);
    USBSerial.print(", Current screen: 0x");
    USBSerial.println((uint32_t)lv_scr_act(), HEX);
    
    // Only log touch-related events
    switch(code) {
        case LV_EVENT_PRESSED: 
            USBSerial.println("Idle screen event: PRESSED"); 
            break;
        case LV_EVENT_PRESSING: 
            USBSerial.println("Idle screen event: PRESSING"); 
            break;
        case LV_EVENT_PRESS_LOST: 
            USBSerial.println("Idle screen event: PRESS_LOST"); 
            break;
        case LV_EVENT_SHORT_CLICKED: 
            USBSerial.println("Idle screen event: SHORT_CLICKED"); 
            break;
        case LV_EVENT_LONG_PRESSED: 
            USBSerial.println("Idle screen event: LONG_PRESSED"); 
            break;
        case LV_EVENT_LONG_PRESSED_REPEAT: 
            USBSerial.println("Idle screen event: LONG_PRESSED_REPEAT"); 
            break;
        case LV_EVENT_CLICKED: 
            USBSerial.println("Idle screen event: CLICKED"); 
            break;
        case LV_EVENT_RELEASED: 
            USBSerial.println("Idle screen event: RELEASED"); 
            break;
        default: 
            // Ignore non-touch events
            break;
    }
    
    if (code != LV_EVENT_CLICKED) return;
    
    // Only allow one click per touch sequence
    if (touchSequenceActive && touchSequenceStartTime == lastClickSequenceTime) {
        USBSerial.println("[IDLE] Ignoring duplicate click in same touch sequence");
        return;
    }
    
    USBSerial.println("\n=== IDLE SCREEN CLICK PROCESSING ===");
    lastClickSequenceTime = touchSequenceStartTime;
    clickProcessedInSequence = true;
    
    State* currentState = stateMachine->getCurrentState();
    if (currentState) {
        const char* stateName = currentState->getStateName();
        
        if (strcmp(stateName, "IdleState") == 0) {
            USBSerial.println(">>> TRANSITION: IdleState -> ProjectSelectState <<<");
            
            // Transition immediately and wait for touch release
            extern StateMachine stateMachine;
            stateMachine.changeState((State*)stateMachine.projectSelectState);
            
            // Prevent further input events until user releases touch
            lv_indev_t* indev = lv_indev_get_act();
            USBSerial.print("[WAIT] Calling lv_indev_wait_release, indev: 0x");
            USBSerial.println((uint32_t)indev, HEX);
            if (indev) {
                lv_indev_wait_release(indev);
                USBSerial.println("[WAIT] wait_release called successfully");
            } else {
                USBSerial.println("[WAIT] ERROR: No active input device!");
            }
        }
    }
}

void UIEventHandler::handleAdjustScreenTap(lv_event_t* e) {
    if (!stateMachine) return;
    
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    USBSerial.println("Adjust screen clicked");
    
    State* currentState = stateMachine->getCurrentState();
    if (currentState) {
        const char* stateName = currentState->getStateName();
        
        if (strcmp(stateName, "AdjustState") == 0) {
            USBSerial.println("Saving duration and returning to IdleState");
            extern StateMachine stateMachine;
            stateMachine.changeState((State*)stateMachine.idleState);
            
            // Prevent further input events until user releases touch
            lv_indev_t* indev = lv_indev_get_act();
            USBSerial.print("[WAIT] Calling lv_indev_wait_release, indev: 0x");
            USBSerial.println((uint32_t)indev, HEX);
            if (indev) {
                lv_indev_wait_release(indev);
                USBSerial.println("[WAIT] wait_release called successfully");
            } else {
                USBSerial.println("[WAIT] ERROR: No active input device!");
            }
        }
    }
}

void UIEventHandler::handleProjectScreenTap(lv_event_t* e) {
    if (!stateMachine) return;
    
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    
    // Ignore phantom touches after screen transitions
    unsigned long timeSinceTransition = millis() - lastScreenTransitionTime;
    if (timeSinceTransition < 200) {
        USBSerial.print("[PROJECT] Ignoring phantom touch, only ");
        USBSerial.print(timeSinceTransition);
        USBSerial.println("ms since screen transition");
        return;
    }
    
    USBSerial.print("[PROJECT] Event target: 0x");
    USBSerial.print((uint32_t)target, HEX);
    USBSerial.print(", Current screen: 0x");
    USBSerial.println((uint32_t)lv_scr_act(), HEX);
    
    // Only log touch-related events
    switch(code) {
        case LV_EVENT_PRESSED: 
            USBSerial.println("Project screen event: PRESSED"); 
            break;
        case LV_EVENT_PRESSING: 
            USBSerial.println("Project screen event: PRESSING"); 
            break;
        case LV_EVENT_PRESS_LOST: 
            USBSerial.println("Project screen event: PRESS_LOST"); 
            break;
        case LV_EVENT_SHORT_CLICKED: 
            USBSerial.println("Project screen event: SHORT_CLICKED"); 
            break;
        case LV_EVENT_LONG_PRESSED: 
            USBSerial.println("Project screen event: LONG_PRESSED"); 
            break;
        case LV_EVENT_LONG_PRESSED_REPEAT: 
            USBSerial.println("Project screen event: LONG_PRESSED_REPEAT"); 
            break;
        case LV_EVENT_CLICKED: 
            USBSerial.println("Project screen event: CLICKED"); 
            break;
        case LV_EVENT_RELEASED: 
            USBSerial.println("Project screen event: RELEASED"); 
            break;
        default: 
            // Ignore non-touch events
            break;
    }
    
    if (code != LV_EVENT_CLICKED) return;
    
    // Only allow one click per touch sequence
    if (touchSequenceActive && touchSequenceStartTime == lastClickSequenceTime) {
        USBSerial.println("[PROJECT] Ignoring duplicate click in same touch sequence");
        return;
    }
    
    USBSerial.println("\n=== PROJECT SCREEN CLICK PROCESSING ===");
    lastClickSequenceTime = touchSequenceStartTime;
    clickProcessedInSequence = true;
    
    State* currentState = stateMachine->getCurrentState();
    if (currentState) {
        const char* stateName = currentState->getStateName();
        
        if (strcmp(stateName, "ProjectSelectState") == 0) {
            USBSerial.println(">>> TRANSITION: ProjectSelectState -> TimerState <<<");
            
            // Save the selected project
            ProjectManager& pm = ProjectManager::getInstance();
            int selectedIndex = screenManager->getCurrentProjectIndex();
            pm.selectProject(selectedIndex);
            USBSerial.print("Selected project: ");
            USBSerial.print(pm.getSelectedProjectName());
            USBSerial.print(" (color: 0x");
            USBSerial.print(pm.getSelectedProjectColor(), HEX);
            USBSerial.println(")");
            
            // Transition immediately and wait for touch release
            extern StateMachine stateMachine;
            stateMachine.changeState((State*)stateMachine.timerState);
            
            // Prevent further input events until user releases touch
            lv_indev_t* indev = lv_indev_get_act();
            USBSerial.print("[WAIT] Calling lv_indev_wait_release, indev: 0x");
            USBSerial.println((uint32_t)indev, HEX);
            if (indev) {
                lv_indev_wait_release(indev);
                USBSerial.println("[WAIT] wait_release called successfully");
            } else {
                USBSerial.println("[WAIT] ERROR: No active input device!");
            }
        }
    }
}

void UIEventHandler::handleTimerScreenTap(lv_event_t* e) {
    if (!stateMachine) return;
    
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    
    // Ignore phantom touches after screen transitions - increased timing
    unsigned long timeSinceTransition = millis() - lastScreenTransitionTime;
    if (timeSinceTransition < 500) {
        USBSerial.print("[TIMER] Ignoring phantom touch, only ");
        USBSerial.print(timeSinceTransition);
        USBSerial.println("ms since screen transition");
        return;
    }
    
    USBSerial.print("[TIMER] Event target: 0x");
    USBSerial.print((uint32_t)target, HEX);
    USBSerial.print(", Current screen: 0x");
    USBSerial.println((uint32_t)lv_scr_act(), HEX);
    
    // Debug: Show what event we received
    const char* eventName = "UNKNOWN";
    switch(code) {
        case LV_EVENT_PRESSED: eventName = "PRESSED"; break;
        case LV_EVENT_PRESSING: eventName = "PRESSING"; break;
        case LV_EVENT_PRESS_LOST: eventName = "PRESS_LOST"; break;
        case LV_EVENT_SHORT_CLICKED: eventName = "SHORT_CLICKED"; break;
        case LV_EVENT_LONG_PRESSED: eventName = "LONG_PRESSED"; break;
        case LV_EVENT_LONG_PRESSED_REPEAT: eventName = "LONG_PRESSED_REPEAT"; break;
        case LV_EVENT_CLICKED: eventName = "CLICKED"; break;
        case LV_EVENT_RELEASED: eventName = "RELEASED"; break;
        default: eventName = "OTHER"; break;
    }
    USBSerial.print("[TIMER] Received event: ");
    USBSerial.println(eventName);
    
    // Manual long press detection using touch timing
    unsigned long currentTime = millis();
    
    if (code == LV_EVENT_PRESSED) {
        // Start tracking for manual long press
        touchStartTime = currentTime;
        longPressDetected = false;
        USBSerial.println("[TIMER] Manual long press tracking started");
        return; // Don't process press events
    }
    
    if (code == LV_EVENT_PRESSING) {
        // Check if we've reached long press threshold
        if (!longPressDetected && (currentTime - touchStartTime >= MANUAL_LONG_PRESS_MS)) {
            longPressDetected = true;
            USBSerial.println("[TIMER] *** MANUAL LONG PRESS DETECTED ***");
            
            // Process as long press immediately
            State* currentState = stateMachine->getCurrentState();
            if (currentState) {
                const char* stateName = currentState->getStateName();
                
                if (strcmp(stateName, "TimerState") == 0 || strcmp(stateName, "PausedState") == 0) {
                    USBSerial.println(">>> MANUAL LONG PRESS: Ending timer -> IdleState <<<");
                    
                    extern StateMachine stateMachine;
                    stateMachine.changeState((State*)stateMachine.idleState);
                    
                    // Reset tracking
                    touchStartTime = 0;
                    longPressDetected = false;
                    return;
                }
            }
        }
        return; // Don't process pressing events as clicks
    }
    
    // Handle clicks only if no long press was detected
    if (code == LV_EVENT_CLICKED) {
        if (longPressDetected) {
            USBSerial.println("[TIMER] Ignoring click - long press already processed");
            longPressDetected = false;
            return;
        }
        USBSerial.println("[TIMER] Processing as short click");
    } else {
        return; // Only handle CLICKED events now
    }
    
    // Only allow one click per touch sequence
    if (touchSequenceActive && touchSequenceStartTime == lastClickSequenceTime) {
        USBSerial.println("[TIMER] Ignoring duplicate click in same touch sequence");
        return;
    }
    
    USBSerial.print("\n=== TIMER/PAUSED SCREEN ");
    USBSerial.print(code == LV_EVENT_LONG_PRESSED ? "LONG PRESS" : "CLICK");
    USBSerial.println(" PROCESSING ===");
    lastClickSequenceTime = touchSequenceStartTime;
    clickProcessedInSequence = true;
    
    State* currentState = stateMachine->getCurrentState();
    if (currentState) {
        const char* stateName = currentState->getStateName();
        
        if (strcmp(stateName, "TimerState") == 0) {
            if (code == LV_EVENT_CLICKED) {
                USBSerial.println(">>> TRANSITION: TimerState -> PausedState <<<");
                
                // Transition immediately and wait for touch release
                extern StateMachine stateMachine;
                stateMachine.changeState((State*)stateMachine.pausedState);
                
                // Prevent further input events until user releases touch
                lv_indev_t* indev = lv_indev_get_act();
                USBSerial.print("[WAIT] Calling lv_indev_wait_release, indev: 0x");
                USBSerial.println((uint32_t)indev, HEX);
                if (indev) {
                    lv_indev_wait_release(indev);
                    USBSerial.println("[WAIT] wait_release called successfully");
                } else {
                    USBSerial.println("[WAIT] ERROR: No active input device!");
                }
            } else if (code == LV_EVENT_LONG_PRESSED) {
                USBSerial.println(">>> LONG PRESS: TimerState -> IdleState (End Timer) <<<");
                
                // End the timer and return to idle
                extern StateMachine stateMachine;
                stateMachine.changeState((State*)stateMachine.idleState);
                
                // Prevent further input events until user releases touch
                lv_indev_t* indev = lv_indev_get_act();
                if (indev) {
                    lv_indev_wait_release(indev);
                }
            }
        } else if (strcmp(stateName, "PausedState") == 0) {
            if (code == LV_EVENT_CLICKED) {
                USBSerial.println("Resuming timer - transitioning to TimerState");
                
                // Transition immediately and wait for touch release
                extern StateMachine stateMachine;
                stateMachine.changeState((State*)stateMachine.timerState);
                
                // Prevent further input events until user releases touch
                lv_indev_t* indev = lv_indev_get_act();
                USBSerial.print("[WAIT] Calling lv_indev_wait_release, indev: 0x");
                USBSerial.println((uint32_t)indev, HEX);
                if (indev) {
                    lv_indev_wait_release(indev);
                    USBSerial.println("[WAIT] wait_release called successfully");
                } else {
                    USBSerial.println("[WAIT] ERROR: No active input device!");
                }
            } else if (code == LV_EVENT_LONG_PRESSED) {
                USBSerial.println(">>> LONG PRESS: PausedState -> IdleState (End Timer) <<<");
                
                // End the timer and return to idle
                extern StateMachine stateMachine;
                stateMachine.changeState((State*)stateMachine.idleState);
                
                // Prevent further input events until user releases touch
                lv_indev_t* indev = lv_indev_get_act();
                if (indev) {
                    lv_indev_wait_release(indev);
                }
            }
        }
    }
}