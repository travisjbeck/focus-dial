#include <Arduino.h>
#include <TFT_eSPI.h>
#include "lv_xiao_round_screen.h"

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 10 && !Serial; i++) {
    delay(100);
  }
  Serial.println("\n\n--- Seeed Official Display Test ---");

  // Initialize the round display using the correct driver
  xiao_disp_init();
  tft.setRotation(0);
  tft.fillScreen(TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setTextSize(2);
  tft.setCursor(10, 100);
  tft.println("Seeed OK?");
  Serial.println("Display test complete.");
}

void loop() {
  // No-op
} 