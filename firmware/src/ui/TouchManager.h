#pragma once

#include <Arduino.h>
#include <esp_log.h>

// Forward declarations
class StateMachine;
class ScreenManager;

// Touch Manager - Release-only touch system to eliminate touch bleeding
class TouchManager {
public:
    TouchManager();
    ~TouchManager();
    
    // Lifecycle
    void init(StateMachine* stateMachine, ScreenManager* screenManager);
    void processTouch(bool touchDetected, int16_t x, int16_t y);
    
    // State management - called by state machine during transitions
    void blockTouchDuringTransition();
    void unblockTouchAfterTransition();
    
    // Configuration
    void setLongPressThreshold(unsigned long ms) { longPressThresholdMs = ms; }
    unsigned long getLongPressThreshold() const { return longPressThresholdMs; }
    
    // Status
    bool isTouchActive() const { return (touchState == PRESSING || touchState == LONG_TRIGGERED); }
    bool isTouchBlocked() const { return (touchState == BLOCKED); }
    
private:
    enum TouchState {
        IDLE,           // No touch detected
        PRESSING,       // Finger down, measuring duration
        LONG_TRIGGERED, // Long press triggered during touch
        BLOCKED         // Touch blocked during state transition
    };
    
    // Touch state tracking
    TouchState touchState;
    unsigned long touchStartTime;
    int16_t touchStartX, touchStartY;
    int16_t lastTouchX, lastTouchY;
    unsigned long longPressThresholdMs;
    
    // Gesture detection
    bool detectSwipeGesture(int16_t currentX, int16_t currentY);
    static const int16_t MIN_SWIPE_DISTANCE = 60; // Minimum pixels for swipe (reduced for easier detection)
    static const int16_t MAX_SWIPE_Y_VARIANCE = 120; // Max vertical variance for horizontal swipe (very forgiving)
    
    // System integration
    StateMachine* stateMachine;
    ScreenManager* screenManager;
    
    // Event processing - only called on touch release
    void handleTouchRelease(unsigned long pressDuration);
    void handleShortPress();
    void handleLongPress();
    void handleLongPressImmediate(); // Triggered during touch, not on release
    
    // Screen-specific logic
    void handleIdleScreenTouch();
    void handleAdjustScreenTouch();
    void handleProjectScreenTouch(); 
    void handleTimerScreenTouch(bool isLongPress);
    void handlePausedScreenTouch(bool isLongPress);
    void handleDoneScreenTouch();
    
    // Confirmation dialog handling
    void handleConfirmDialogTouch();
    
    // Current screen detection
    const char* getCurrentScreenName() const;
    
    // Logging
    const char* getLogTag() const { return "TouchManager"; }
    void logTouchEvent(const char* event, unsigned long duration = 0) const;
};