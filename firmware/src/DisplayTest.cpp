#include <Arduino.h>
#include <TFT_eSPI.h> // Main graphics library
#include <Wire.h>     // Not strictly needed for this TFT-only test, but good to keep for structure

#define TFT_MANUAL_RST_PIN 2 // Defined as D1 / GPIO2 in platformio.ini build flags (-DTFT_RST=2)

TFT_eSPI tft = TFT_eSPI(); // TFT_eSPI MISO Global object

void setup() {
  Serial.begin(115200);
  // A small delay to allow serial monitor to connect after reset, especially for native USB
  for (int i = 0; i < 10 && !Serial; i++) {
    delay(100);
  }

  Serial.println("\n\n--- TFT ONLY TEST V2 (Manual RST) ---");

#ifdef TFT_MANUAL_RST_PIN
  Serial.print("Manually resetting TFT on pin: "); Serial.println(TFT_MANUAL_RST_PIN);
  pinMode(TFT_MANUAL_RST_PIN, OUTPUT);
  digitalWrite(TFT_MANUAL_RST_PIN, LOW);
  delay(20); // Keep reset low for a short period
  digitalWrite(TFT_MANUAL_RST_PIN, HIGH);
  delay(150); // Wait for display to recover from reset
  Serial.println("Manual TFT reset complete.");
#endif

  Serial.println("Calling tft.init()...");
  tft.init();
  Serial.println("tft.init() returned.");

  Serial.println("Setting rotation to 0 (default)...");
  tft.setRotation(0); // Default portrait mode for round display
  Serial.println("Rotation set.");

  Serial.println("Calling tft.fillScreen(TFT_RED)... // Changed from BLUE");
  tft.fillScreen(TFT_RED);
  Serial.println("tft.fillScreen(TFT_RED) returned.");

  Serial.println("Setting up text properties...");
  tft.setTextColor(TFT_WHITE, TFT_RED); // White text, RED background
  tft.setTextSize(2); // Default font size x 2
  tft.setCursor(10, 100); // Approx center for "Hello?"
  Serial.println("Text properties set.");

  Serial.println("Printing 'Hello?' to TFT...");
  tft.println("Hello?");
  Serial.println("TFT print 'Hello?' returned.");

  Serial.println("--- Setup complete ---  (V2 Manual RST)");
}

void loop() {
  tft.setCursor(10, 130); // Below "Hello?"
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Ensure background is cleared for this small text
  tft.fillRect(10, 130, 100, 16, TFT_BLACK); // Clear area for millis
  tft.print("Millis: ");
  tft.print(millis() / 1000);
  
  Serial.print("Loop (V2): Updated millis on TFT. Serial uptime: ");
  Serial.println(millis() / 1000);
  delay(1000);
} 