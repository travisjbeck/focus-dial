#include <Arduino.h>

// Remove redundant define; build flag sets USE_TFT_ESPI_LIBRARY
// #define USE_TFT_ESPI_LIBRARY

#include <lvgl.h>
#include "lv_xiao_round_screen.h"

// Note: TFT_eSPI tft; is already instantiated globally inside lv_xiao_round_screen.h

void setup() {
  Serial.begin(115200);
  // Add a small delay to allow serial monitor to connect
  delay(2000); 
  Serial.println("--- LVGL Minimal Display Test ---");

  // Initialize LVGL core
  lv_init();
#if LVGL_VERSION_MAJOR == 9
  lv_tick_set_cb([](){ return (uint32_t)millis(); });
#endif

  // Initialize display and (optionally) touch for LVGL
  lv_xiao_disp_init();
  // lv_xiao_touch_init(); // touch not required for this simple test

  // Fill screen via Arduino_GFX driver
  gfx->begin();
  gfx->setRotation(0);
  gfx->fillScreen(BLUE);
  Serial.println("TFT fillScreen done");

  // Also draw a basic LVGL label so we verify LVGL draw pipeline
  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello Round Display!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  Serial.println("Setup complete.");
}

void loop() {
  lv_timer_handler();
  delay(5);
} 