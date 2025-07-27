#pragma once

#include <lvgl.h>

// Forward declaration
class ScreenManager;
class StateMachine;

class UIEventHandler {
public:
    static void init(ScreenManager* screenMgr, StateMachine* stateMachine);
    
    // Event callbacks
    static void handleIdleScreenTap(lv_event_t* e);
    static void handleAdjustScreenTap(lv_event_t* e);
    static void handleProjectScreenTap(lv_event_t* e);
    static void handleTimerScreenTap(lv_event_t* e);
    
private:
    static ScreenManager* screenManager;
    static StateMachine* stateMachine;
};