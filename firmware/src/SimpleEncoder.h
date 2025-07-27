#pragma once

#include <Arduino.h>
#include <RotaryEncoder.h>

class SimpleEncoder {
public:
    SimpleEncoder(int pinA, int pinB);
    ~SimpleEncoder();
    
    void begin();
    void tick(); // Call this from ISR
    int getPosition();
    void setPosition(int newPosition);
    int readDelta(); // Returns change since last read and resets
    
    // Static ISR handlers
    static void handleInterruptA();
    static void handleInterruptB();
    
private:
    RotaryEncoder* encoder;
    int lastPosition;
    int pinA;
    int pinB;
    
    static SimpleEncoder* instance; // For ISR access
};

// Global instance for ISR
extern SimpleEncoder simpleEncoder;