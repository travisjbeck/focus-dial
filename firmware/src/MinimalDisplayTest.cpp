#include <Arduino.h>

// Remove redundant define; build flag sets USE_TFT_ESPI_LIBRARY
// #define USE_TFT_ESPI_LIBRARY

#include <lvgl.h>
#include "lv_xiao_round_screen.h"
#include <RotaryEncoder.h> // Include the Rotary Encoder library
#include <Adafruit_NeoPixel.h> // Include the NeoPixel library
// #include <math.h> // No longer needed for block positioning
#include <PCF8563.h> // Include RTC library

// Define Encoder Pins
#define ENCODER_PIN_A 43 // XIAO D6 maps to GPIO43
#define ENCODER_PIN_B 8  // XIAO D9 maps to GPIO8

// Define NeoPixel LED Ring
#define LED_PIN 1 // D0 maps to GPIO1
#define NUM_LEDS 16

// Setup a RotaryEncoder object
// Revert to TWO03 mode, which was previously better
RotaryEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, RotaryEncoder::LatchMode::TWO03);

// Global pointer for the ISR
static RotaryEncoder* encoderISRPtr = nullptr;

// Global pointer for the encoder indicator arc on the display
static lv_obj_t *progress_arc = NULL; // Changed from encoder_block

// Global RTC object (Using the correct class name)
PCF8563_Class rtc; // Corrected class name

// Setup NeoPixel object
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ISR function
// IRAM_ATTR recommended for ESP32 ISRs for speed
void IRAM_ATTR handleEncoderInterrupt_minimal() {
  if (encoderISRPtr) {
    encoderISRPtr->tick();
  }
}

// Note: TFT_eSPI tft; is already instantiated globally inside lv_xiao_round_screen.h

// Note: The Round Display library likely uses CST816S touch controller via I2C (GPIO5/D4 SDA, GPIO6/D5 SCL)

void setup() {
  Serial.begin(115200);
  // Add a small delay to allow serial monitor to connect
  delay(2000); 
  Serial.println("--- LVGL Minimal Display & Touch Test ---");

  // Initialize LVGL core
  lv_init();
#if LVGL_VERSION_MAJOR == 9
  lv_tick_set_cb([](){ return (uint32_t)millis(); });
#endif

  // Initialize display and touch for LVGL
  lv_xiao_disp_init();
  lv_xiao_touch_init(); // Enable touch initialization

  // Fill screen via Arduino_GFX driver
  gfx->begin();
  gfx->setRotation(0);
  gfx->fillScreen(BLUE);
  Serial.println("TFT fillScreen done");

  // Also draw a basic LVGL label so we verify LVGL draw pipeline
  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello! Touch/Turn!"); // Update text slightly
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  // --- Encoder Initialization ---
  encoderISRPtr = &encoder; // Set ISR pointer to our encoder instance
  pinMode(ENCODER_PIN_A, INPUT_PULLUP); // Ensure pins are Input with Pullup
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), handleEncoderInterrupt_minimal, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), handleEncoderInterrupt_minimal, CHANGE);
  Serial.println("Encoder interrupts attached.");
  // ---------------------------

  // --- NeoPixel Initialization & Test ---
  leds.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  leds.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  leds.clear();           // Set all pixel colors to 'off'
  leds.setPixelColor(0, leds.Color(0, 150, 0)); // Set first pixel to green
  leds.show();            // Send the updated pixel colors to the hardware.
  Serial.println("NeoPixel initialized and first LED set to green.");
  // -----------------------------------

  // --- Create Encoder Indicator Arc ---
  progress_arc = lv_arc_create(lv_screen_active());
  lv_obj_set_size(progress_arc, 230, 230); // Increase size slightly to be closer to edge
  lv_obj_align(progress_arc, LV_ALIGN_CENTER, 0, 0); // Center it
  lv_arc_set_rotation(progress_arc, 270); // Rotate so 0 degrees is at the top (12 o'clock)
  lv_arc_set_bg_angles(progress_arc, 0, 360); // Set background arc to be a full circle
  lv_arc_set_range(progress_arc, 0, 360); // Set the range for the value (0-360 degrees)
  lv_arc_set_value(progress_arc, 0); // Start empty

  // Style the arc
  lv_obj_remove_style(progress_arc, NULL, LV_PART_KNOB | LV_STATE_DEFAULT); // Remove the knob
  // lv_obj_remove_style(progress_arc, NULL, LV_PART_MAIN | LV_STATE_DEFAULT); // Don't remove main style, style it instead
  
  // Style the background arc part (the track)
  lv_obj_set_style_arc_color(progress_arc, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT); // Dark grey background
  lv_obj_set_style_arc_width(progress_arc, 10, LV_PART_MAIN | LV_STATE_DEFAULT); // Same width as indicator

  // Style the indicator part (the filled arc)
  lv_obj_set_style_arc_color(progress_arc, lv_color_hex(0x00FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT); // Green indicator
  lv_obj_set_style_arc_width(progress_arc, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT); // Set indicator thickness
  // -----------------------------------

  // --- RTC Initialization & Test ---
  Wire.begin(5, 6); // Ensure I2C is started (SDA=GPIO5/D4, SCL=GPIO6/D5)
  rtc.begin(); // Initialize RTC (often doesn't return a boolean)
  
  // Get current time
  RTC_Date datetime = rtc.getDateTime(); // Use correct struct

  // Basic validity check (e.g., year >= 2024)
  if (datetime.year < 2024) { 
      Serial.println("RTC time appears invalid (year < 2024). Consider setting the time.");
      // Setting time from compile time requires parsing __DATE__ and __TIME__
      // For this test, we'll just report it's likely unset.
      // Example of manual setting (replace with actual desired time):
      // rtc.setDateTime(2024, 7, 29, 10, 0, 0); // Note: Needs 6 args: Y,M,D, H,M,S
  } else {
      Serial.println("RTC time seems valid (year >= 2024).");
  }

  // Print the time regardless of validity check for debugging
  char buf[100];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
          datetime.year,
          datetime.month,
          datetime.day,
          datetime.hour,
          datetime.minute,
          datetime.second);
  Serial.printf("Current RTC Reading: %s\n", buf);
  // ---------------------------------

  Serial.println("Setup complete. Touch the screen or turn the encoder.");
}

// Global variables to store last touch coordinates and encoder position
int16_t last_touch_x = -1;
int16_t last_touch_y = -1;
long last_encoder_pos = 0; // Store the last known encoder position
int last_led_index = 0; // Store the index of the last lit LED

void loop() {
  lv_timer_handler();

  // --- Encoder Reading (ISR handles tick(), we just check position) ---
  // encoder.tick(); // <<< REMOVE THIS - Handled by ISR >>>

  long current_encoder_pos = encoder.getPosition();
  if (current_encoder_pos != last_encoder_pos) {
    // Determine direction based on position change
    const char* direction_str;
    if (current_encoder_pos > last_encoder_pos) {
      direction_str = "CW";
    } else {
      direction_str = "CCW";
    }
    Serial.printf("Encoder Position: %ld, Direction: %s\n", 
                  current_encoder_pos,
                  direction_str);

    // --- Update LED based on encoder ---
    // Calculate new LED index, ensuring it's positive and wraps around
    int current_led_index = (current_encoder_pos % NUM_LEDS + NUM_LEDS) % NUM_LEDS; 

    if (current_led_index != last_led_index) {
        leds.setPixelColor(last_led_index, leds.Color(0, 0, 0)); // Turn off old LED
        leds.setPixelColor(current_led_index, leds.Color(0, 150, 0)); // Turn on new LED (green)
        leds.show(); // Update the strip

        // --- Update display arc value ---
        // Map the 0-15 LED index to 0-360 degrees
        int arc_value = (int)(((float)current_led_index / NUM_LEDS) * 360.0f); 
        // Ensure value stays within 0-360 range, though mapping should handle it.
        arc_value = constrain(arc_value, 0, 360); 
        lv_arc_set_value(progress_arc, arc_value);
        // -------------------------------

        last_led_index = current_led_index; // Store the new index
    }
    // ---------------------------------

    last_encoder_pos = current_encoder_pos;
  }

  // Directly check touch status using the display object if available from the helper,
  // or use the underlying touch library's functions.
  // Assuming 'display' object is globally accessible or part of the helper structure.
  // The lv_xiao_round_screen likely uses Seeed_Arduino_RoundDisplay internally.
  
  // Since lv_xiao_touch_init() is called, LVGL's input system should be active.
  // Let's try the LVGL way to get the last point coordinates:
  lv_point_t point;
  lv_indev_t *touch_indev = lv_indev_get_next(NULL); // Get the default input device (should be touch)
  
  if (touch_indev && lv_indev_get_type(touch_indev) == LV_INDEV_TYPE_POINTER) {
    lv_indev_get_point(touch_indev, &point);
    lv_indev_state_t state = lv_indev_get_state(touch_indev);

    // --- Add coordinate validity check ---
    bool coordinates_valid = (point.x >= 0 && point.x < 240 && point.y >= 0 && point.y < 240);

    if (state == LV_INDEV_STATE_PRESSED) {
        // Only process if coordinates are valid and changed
        if (coordinates_valid && (point.x != last_touch_x || point.y != last_touch_y)) {
            Serial.printf("Touch State: Pressed , X: %d, Y: %d\n", point.x, point.y);
            
            // Erase the last dot before drawing a new one
            if (last_touch_x != -1) { // Check if there was a previous valid touch
                 gfx->fillCircle(last_touch_x, last_touch_y, 3, BLUE); 
            }

            gfx->fillCircle(point.x, point.y, 3, YELLOW); // Draw a new yellow dot
            last_touch_x = point.x;
            last_touch_y = point.y;
        }
    } else if (state == LV_INDEV_STATE_RELEASED) {
        // Only process release if the last recorded press coordinates were valid
        if (last_touch_x != -1 || last_touch_y != -1) {
             // Use last valid coordinates for the release message
            Serial.printf("Touch State: Released, X: %d, Y: %d\n", last_touch_x, last_touch_y);
            gfx->fillCircle(last_touch_x, last_touch_y, 3, BLUE); // Clear the dot
            last_touch_x = -1; // Reset last coordinates
            last_touch_y = -1;
        }
    } else { 
        // Handle case where state is neither pressed nor released after a press (e.g., invalid state)
        // Or simply ensure dot is cleared if touch tracking somehow got lost
        if (last_touch_x != -1 || last_touch_y != -1) {
             gfx->fillCircle(last_touch_x, last_touch_y, 3, BLUE); // Ensure dot is cleared
            last_touch_x = -1;
            last_touch_y = -1;
            // Optionally print a message indicating an unexpected state or reset
            // Serial.println("Touch state invalid/reset, clearing dot.");
        }
    }
  }

  delay(2); // Reduce delay significantly for faster encoder polling
} 