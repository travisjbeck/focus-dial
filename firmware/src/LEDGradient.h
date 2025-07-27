#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "BrandColors.h"

class LEDGradient {
public:
    // Create an alpha gradient on the LED ring
    static void showAlphaGradient(Adafruit_NeoPixel& leds, int offset = 0) {
        int numLeds = leds.numPixels();
        
        for (int i = 0; i < numLeds; i++) {
            // Calculate position with offset for rotation
            int pos = (i + offset) % numLeds;
            
            // Calculate alpha based on position (0-255)
            // Full brightness at position 0, fading to off
            int alpha = 255 - (i * 255 / numLeds);
            
            // Apply alpha to brand color
            uint8_t r = (BRAND_COLOR_PRIMARY_R * alpha) / 255;
            uint8_t g = (BRAND_COLOR_PRIMARY_G * alpha) / 255;
            uint8_t b = (BRAND_COLOR_PRIMARY_B * alpha) / 255;
            
            leds.setPixelColor(pos, r, g, b);
        }
        
        leds.show();
    }
    
    // Show timer duration as a filled arc
    static void showDurationArc(Adafruit_NeoPixel& leds, int minutes, int maxMinutes = 60) {
        int numLeds = leds.numPixels();
        
        // Calculate how many LEDs to light based on duration
        int ledsToLight = (minutes * numLeds) / maxMinutes;
        if (ledsToLight > numLeds) ledsToLight = numLeds;
        
        for (int i = 0; i < numLeds; i++) {
            if (i < ledsToLight) {
                // Calculate alpha gradient for lit LEDs
                int alpha = 255 - (i * 128 / ledsToLight); // Fade to 50% at the end
                uint8_t r = (BRAND_COLOR_PRIMARY_R * alpha) / 255;
                uint8_t g = (BRAND_COLOR_PRIMARY_G * alpha) / 255;
                uint8_t b = (BRAND_COLOR_PRIMARY_B * alpha) / 255;
                leds.setPixelColor(i, r, g, b);
            } else {
                // Off for remaining LEDs
                leds.setPixelColor(i, 0, 0, 0);
            }
        }
        
        leds.show();
    }
};