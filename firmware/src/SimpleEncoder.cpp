#include "SimpleEncoder.h"

// Static member initialization
SimpleEncoder* SimpleEncoder::instance = nullptr;

// Global instance
SimpleEncoder simpleEncoder(17, 18); // Using pins from pin_config.h

SimpleEncoder::SimpleEncoder(int pinA, int pinB) : 
    encoder(nullptr), 
    lastPosition(0),
    pinA(pinA), 
    pinB(pinB) {
    instance = this;
}

SimpleEncoder::~SimpleEncoder() {
    if (encoder) {
        delete encoder;
    }
}

void SimpleEncoder::begin() {
    // Create encoder with TWO03 latch mode for mechanical encoders
    encoder = new RotaryEncoder(pinA, pinB, RotaryEncoder::LatchMode::TWO03);
    
    // Set up interrupts on both pins
    attachInterrupt(digitalPinToInterrupt(pinA), handleInterruptA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pinB), handleInterruptB, CHANGE);
    
    lastPosition = 0;
}

void SimpleEncoder::tick() {
    if (encoder) {
        encoder->tick();
    }
}

int SimpleEncoder::getPosition() {
    if (encoder) {
        return encoder->getPosition();
    }
    return 0;
}

void SimpleEncoder::setPosition(int newPosition) {
    if (encoder) {
        encoder->setPosition(newPosition);
        lastPosition = newPosition;
    }
}

int SimpleEncoder::readDelta() {
    if (!encoder) return 0;
    
    int currentPosition = encoder->getPosition();
    int delta = currentPosition - lastPosition;
    lastPosition = currentPosition;
    return delta;
}

// Static ISR handlers
void IRAM_ATTR SimpleEncoder::handleInterruptA() {
    if (instance) {
        instance->tick();
    }
}

void IRAM_ATTR SimpleEncoder::handleInterruptB() {
    if (instance) {
        instance->tick();
    }
}