#include "controllers/DisplayController.h"

#include "fonts/Picopixel.h"
#include "fonts/Org_01.h"
#include "bitmaps.h"
#include <Fonts/FreeSansBold9pt7b.h>
#include <lvgl.h>

DisplayController::DisplayController()
    // : oled(oledWidth, oledHeight, &Wire, -1), animation(&oled) // Temporarily commented out for LVGL migration - Phase 1
    : animation() // Temporarily modified for LVGL migration - Phase 1
{}

void DisplayController::begin()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // All OLED-specific initialization is removed.
  // This function can be used for LVGL-specific setup if needed later,
  // but for now, LVGL is initialized in main.cpp.
  Serial.println("DisplayController::begin() called (no-op for now).");
}

void DisplayController::drawSplashScreen()
{
  Serial.println("DisplayController::drawSplashScreen called (ENTRY POINT)");
  if (lv_screen_active()) {
    Serial.println("DisplayController::drawSplashScreen - lv_screen_active() is TRUE");
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "SPLASH SCREEN NOW!"); // Changed text for clear identification
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    Serial.println("DisplayController::drawSplashScreen - new label created.");
    lv_refr_now(NULL); // Ensure splash refreshes
  } else {
    Serial.println("DisplayController::drawSplashScreen - lv_screen_active() is FALSE");
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // oled.clearDisplay();
  // oled.drawBitmap(16, 3, focusdial_logo, 99, 45, 1);
  // oled.setTextColor(1);
  // oled.setTextSize(1);
  // oled.setFont(&Picopixel);
  // oled.setCursor(21, 60);
  // oled.print("YOUTUBE/ @SALIMBENBOUZ");
  // oled.display();
}

void DisplayController::drawIdleScreen(int durationMinutes, bool wifi)
{
  Serial.println("DisplayController::drawIdleScreen called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "Screen: Idle\nDuration: %d\nWiFi: %s", durationMinutes, wifi ? "On" : "Off");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL);
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // if (isAnimationRunning())
  //   return;
  // static unsigned long lastBlinkTime = 0;
  // static bool blinkState = true;
  // unsigned long currentTime = millis();
  // if (!wifi && (currentTime - lastBlinkTime >= 500))
  // {
  //   blinkState = !blinkState;
  //   lastBlinkTime = currentTime;
  // }
  // oled.clearDisplay();
  // oled.setFont(&Picopixel);
  // oled.setTextSize(1);
  // oled.setTextColor(1);
  // oled.setCursor(40, 58);
  // oled.print("PRESS TO START");
  // oled.drawRoundRect(35, 51, 60, 11, 1, 1);
  // if (wifi)
  // {
  //   oled.drawBitmap(70, 3, icon_wifi_on, 5, 5, 1);
  //   oled.setCursor(54, 7);
  //   oled.print("WIFI");
  // }
  // else if (blinkState)
  // {
  //   oled.drawBitmap(70, 3, icon_wifi_off, 5, 5, 1);
  //   oled.setCursor(54, 7);
  //   oled.print("WIFI");
  // }
  // if (durationMinutes == 0)
  // {
  //   int iconWidth = 48;
  //   int iconHeight = 24;
  //   int x = (oled.width() - iconWidth) / 2;
  //   int y = (oled.height() - iconHeight) / 2;
  //   oled.drawBitmap(x, y, icon_infinity, iconWidth, iconHeight, 1);
  // }
  // else
  // {
  //   char left[3], right[3];
  //   int xLeft = 1;
  //   int xRight = 73;
  //   sprintf(left, "%02d", durationMinutes);
  //   strcpy(right, "00");
  //   if (left[0] == '1')
  //   {
  //     xLeft += 20;
  //   }
  //   oled.setTextSize(5);
  //   oled.setFont(&Org_01);
  //   oled.setCursor(xLeft, 36);
  //   oled.print(left);
  //   oled.setCursor(xRight, 36);
  //   oled.print(right);
  //   oled.fillRect(62, 21, 5, 5, 1);
  //   oled.fillRect(62, 31, 5, 5, 1);
  // }
  // oled.display();
}

void DisplayController::drawTimerScreen(int timeValue, bool isCountUp)
{
  Serial.println("DisplayController::drawTimerScreen called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "Screen: Timer\nTime: %d\nCountUp: %s", timeValue, isCountUp ? "Yes" : "No");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL);
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // if (isAnimationRunning())
  //   return;
  // oled.clearDisplay();
  // int displaySeconds = timeValue;
  // if (!isCountUp && displaySeconds < 0)
  // {
  //   displaySeconds = 0;
  // }
  // else if (isCountUp && displaySeconds < 0)
  // {
  //   displaySeconds = 0;
  // }
  // int hours = displaySeconds / 3600;
  // int minutes = (displaySeconds % 3600) / 60;
  // int seconds = displaySeconds % 60;
  // char left[3], right[3];
  // int xLeft = 1;
  // int xRight = 73;
  // int yPos = 36;
  // if (hours > 0 || (isCountUp && displaySeconds >= 3600))
  // {
  //   sprintf(left, "%02d", hours);
  //   sprintf(right, "%02d", minutes);
  // }
  // else
  // {
  //   sprintf(left, "%02d", minutes);
  //   sprintf(right, "%02d", seconds);
  //   yPos = 40;
  // }
  // if (left[0] == '1')
  // {
  //   xLeft += 20;
  // }
  // if (right[0] == '1')
  // {
  //   xRight += 20;
  // }
  // oled.setTextColor(1);
  // oled.setTextSize(5);
  // oled.setFont(&Org_01);
  // oled.setCursor(xLeft, yPos);
  // oled.print(left);
  // oled.setCursor(xRight, yPos);
  // oled.print(right);
  // oled.fillRect(62, yPos - 15, 5, 5, 1);
  // oled.fillRect(62, yPos - 5, 5, 5, 1);
  // oled.setTextSize(1);
  // oled.setFont(&Picopixel);
  // if (hours > 0 || (isCountUp && displaySeconds >= 3600))
  // {
  //   oled.setCursor(27, 54);
  //   oled.print("H");
  //   oled.setCursor(98, 54);
  //   oled.print("M");
  // }
  // else
  // {
  //   oled.setCursor(27, 54);
  //   oled.print("M");
  //   oled.setCursor(98, 54);
  //   oled.print("S");
  // }
  // oled.display();
}

void DisplayController::drawPausedScreen(int remainingSeconds)
{
  Serial.println("DisplayController::drawPausedScreen called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "Screen: Paused\nRemaining: %ds", remainingSeconds);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL);
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // if (isAnimationRunning())
  //   return;
  // oled.clearDisplay();
  // if (remainingSeconds < 0)
  // {
  //   remainingSeconds = 0;
  // }
  // int hours = remainingSeconds / 3600;
  // int minutes = (remainingSeconds % 3600) / 60;
  // int seconds = remainingSeconds % 60;
  // char left[3], right[3];
  // int xLeft = 1;
  // int xRight = 73;
  // if (hours > 0)
  // {
  //   sprintf(left, "%02d", hours);
  //   sprintf(right, "%02d", minutes);
  // }
  // else
  // {
  //   sprintf(left, "%02d", minutes);
  //   sprintf(right, "%02d", seconds);
  // }
  // if (left[0] == '1')
  // {
  //   xLeft += 20;
  // }
  // if (right[0] == '1')
  // {
  //   xRight += 20;
  // }
  // oled.setTextColor(1);
  // oled.setTextSize(5);
  // oled.setFont(&Org_01);
  // oled.setCursor(xLeft, 40);
  // oled.print(left);
  // oled.setCursor(xRight, 40);
  // oled.print(right);
  // oled.fillRect(62, 25, 5, 5, 1);
  // oled.fillRect(62, 35, 5, 5, 1);
  // oled.setTextSize(1);
  // oled.setFont(&Picopixel);
  // if (hours > 0)
  // {
  //   oled.setCursor(27, 54);
  //   oled.print("H");
  //   oled.setCursor(98, 54);
  //   oled.print("M");
  // }
  // else
  // {
  //   oled.setCursor(27, 54);
  //   oled.print("M");
  //   oled.setCursor(98, 54);
  //   oled.print("S");
  // }
  // oled.drawBitmap(100, 3, icon_pause, 24, 24, 1);
  // oled.display();
}

void DisplayController::drawResetScreen(bool resetSelected)
{
  Serial.println("DisplayController::drawResetScreen called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "Screen: Reset\nSelected: %s", resetSelected ? "Yes" : "No");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL);
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // oled.clearDisplay();
  // oled.setTextColor(1);
  // oled.setTextSize(1);
  // oled.setFont(&Picopixel);
  // oled.setCursor(45, 20);
  // oled.print("RESET TIMER?");
  // int buttonWidth = 50;
  // int buttonHeight = 15;
  // int spacing = 10;
  // int yPos = 35;
  // int xYes = (oled.width() / 2) - buttonWidth - (spacing / 2);
  // int xNo = (oled.width() / 2) + (spacing / 2);
  // if (resetSelected)
  // {
  //   oled.fillRect(xYes, yPos, buttonWidth, buttonHeight, 1);
  //   oled.setTextColor(0);
  //   oled.setCursor(xYes + 15, yPos + 9);
  //   oled.print("YES");
  //   oled.setTextColor(1);
  //   oled.drawRect(xNo, yPos, buttonWidth, buttonHeight, 1);
  //   oled.setCursor(xNo + 18, yPos + 9);
  //   oled.print("NO");
  // }
  // else
  // {
  //   oled.drawRect(xYes, yPos, buttonWidth, buttonHeight, 1);
  //   oled.setCursor(xYes + 15, yPos + 9);
  //   oled.print("YES");
  //   oled.fillRect(xNo, yPos, buttonWidth, buttonHeight, 1);
  //   oled.setTextColor(0);
  //   oled.setCursor(xNo + 18, yPos + 9);
  //   oled.print("NO");
  // }
  // oled.display();
}

void DisplayController::drawDoneScreen(unsigned long finalElapsedTime)
{
  Serial.println("DisplayController::drawDoneScreen called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "Screen: Done\nElapsed: %lus", finalElapsedTime);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL);
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // oled.clearDisplay();
  // int hours = finalElapsedTime / 3600;
  // int minutes = (finalElapsedTime % 3600) / 60;
  // int seconds = finalElapsedTime % 60;
  // oled.setTextColor(1);
  // oled.setTextSize(1);
  // oled.setFont(&Picopixel);
  // oled.setCursor(30, 10);
  // oled.print("SESSION COMPLETED!");
  // char timeStr[12];
  // if (hours > 0)
  // {
  //   sprintf(timeStr, "%dh %02dm %02ds", hours, minutes, seconds);
  // }
  // else
  // {
  //   sprintf(timeStr, "%dm %02ds", minutes, seconds);
  // }
  // int textWidth = strlen(timeStr) * 6; // Approximate width with Picopixel font
  // oled.setCursor((oled.width() - textWidth) / 2, 25);
  // oled.print(timeStr);
  // oled.setFont(&FreeSansBold9pt7b);
  // oled.setTextSize(1);
  // const char *doneText = "DONE";
  // int16_t x1, y1;
  // uint16_t w, h;
  // oled.getTextBounds(doneText, 0, 0, &x1, &y1, &w, &h);
  // oled.setCursor((oled.width() - w) / 2, 55);
  // oled.print(doneText);
  // oled.display();
}

void DisplayController::drawAdjustScreen(int duration, bool wifi)
{
  Serial.println("DisplayController::drawAdjustScreen called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "Screen: Adjust\nDuration: %d\nWiFi: %s", duration, wifi ? "On" : "Off");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL);
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // oled.clearDisplay();
  // char durationStr[6];
  // sprintf(durationStr, "%d min", duration);
  // oled.setTextColor(1);
  // oled.setTextSize(1);
  // oled.setFont(&Picopixel);
  // int textWidth = strlen(durationStr) * 6;
  // oled.setCursor((oled.width() - textWidth) / 2, 25);
  // oled.print(durationStr);
  // oled.setCursor(35, 50);
  // oled.print("TURN TO ADJUST");
  // oled.setCursor(38, 60);
  // oled.print("PRESS TO SAVE");
  // if (wifi)
  // {
  //   oled.drawBitmap(118, 3, icon_wifi_on, 5, 5, 1);
  // }
  // else
  // {
  //   oled.drawBitmap(118, 3, icon_wifi_off, 5, 5, 1);
  // }
  // oled.display();
}

void DisplayController::drawProvisionScreen()
{
  Serial.println("DisplayController::drawProvisionScreen called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "Screen: Provision");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL); // Attempt to force an immediate refresh
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // oled.clearDisplay();
  // oled.setTextColor(1);
  // oled.setTextSize(1);
  // oled.setFont(&Picopixel);
  // oled.setCursor(20, 10);
  // oled.print("FocusDial Provision");
  // oled.setCursor(10, 25);
  // oled.printf("SSID: %s", WiFi.softAPSSID().c_str());
  // oled.setCursor(10, 35);
  // oled.printf("IP: %s", WiFi.softAPIP().toString().c_str());
  // oled.setCursor(10, 45);
  // oled.print("Connect & scan QR or");
  // oled.setCursor(10, 55);
  // oled.print("visit IP to configure.");
  // oled.display();
}

void DisplayController::clear()
{
  Serial.println("DisplayController::clear called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    // Optionally, set a background color or a "Cleared" label
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text_fmt(label, "Screen: Cleared");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL);
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // oled.clearDisplay();
  // oled.display();
}

void DisplayController::showAnimation(const byte frames[][288], int frameCount, bool loop, bool reverse, unsigned long durationMs, int width, int height)
{
  // Temporarily commented out for LVGL migration - Phase 1
  // animation.start(frames, frameCount, loop, reverse, durationMs, width, height);
}

void DisplayController::updateAnimation()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // if (animation.isRunning())
  // {
  //   animation.update();
  // }
}

bool DisplayController::isAnimationRunning()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // return animation.isRunning();
  return false; // Default to false as animations are disabled
}

void DisplayController::showConfirmation()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // showAnimation(checkmark_animation_frames, CHECKMARK_ANIMATION_FRAMES, false, false, 1000);
}

void DisplayController::showCancel()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // showAnimation(cancel_animation_frames, CANCEL_ANIMATION_FRAMES, false, false, 1000);
}

void DisplayController::showReset()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // showAnimation(reset_animation_frames, RESET_ANIMATION_FRAMES, false, false, 1000);
}

void DisplayController::showConnected()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // showAnimation(connected_animation_frames, CONNECTED_ANIMATION_FRAMES, false, false, 1000);
}

void DisplayController::showTimerDone()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // showAnimation(timer_done_animation_frames, TIMER_DONE_ANIMATION_FRAMES, false, false, 1000);
}

void DisplayController::showTimerStart()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // showAnimation(timer_start_animation_frames, TIMER_START_ANIMATION_FRAMES, true, false, 2000); // Loop for 2 seconds
}

void DisplayController::showTimerPause()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // showAnimation(timer_pause_animation_frames, TIMER_PAUSE_ANIMATION_FRAMES, false, false, 1000);
}

void DisplayController::showTimerResume()
{
  // Temporarily commented out for LVGL migration - Phase 1
  // showAnimation(timer_resume_animation_frames, TIMER_RESUME_ANIMATION_FRAMES, false, false, 1000);
}

void DisplayController::drawProjectSelectionScreen(const ProjectList &projects, int selectedIndex, int topIndex, int numToShow)
{
  Serial.println("DisplayController::drawProjectSelectionScreen called");
  if (lv_screen_active()) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t* label = lv_label_create(lv_screen_active());
    // We can make this more informative later if needed
    lv_label_set_text_fmt(label, "Screen: Project Select\nSelected: %d\nTop: %d", selectedIndex, topIndex);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL);
  }
  // Temporarily commented out for LVGL migration - Phase 1
  // oled.clearDisplay();
  // oled.setTextColor(SSD1306_WHITE);
  // oled.setTextWrap(false);
  //
  // // --- Draw Title in a Box ---
  // oled.setFont(&Picopixel);
  // oled.setTextSize(1);
  // const char *title = "SELECT PROJECT";
  // int16_t tx1, ty1, titleX, titleY;
  // uint16_t tw, th;
  // oled.getTextBounds(title, 0, 0, &tx1, &ty1, &tw, &th);
  // titleX = (oled.width() - tw) / 2; // Center horizontally
  // titleY = 8;                       // Set Y position for text baseline
  // oled.setCursor(titleX, titleY);
  // oled.print(title);
  // // Draw rounded box around title
  // int boxPaddingX = 3;
  // int boxPaddingY_Top = 2;
  // int boxPaddingY_Bottom = 3;               // Increase bottom padding
  // int boxY = titleY - th - boxPaddingY_Top; // Box top Y
  // oled.drawRoundRect(titleX - boxPaddingX, boxY, tw + (2 * boxPaddingX), th + boxPaddingY_Top + boxPaddingY_Bottom + 1, 1, SSD1306_WHITE);
  //
  // // Check if the selected index is valid
  // if (selectedIndex < 0 || selectedIndex >= projects.size())
  // {
  //   oled.setFont(); // Reset to default GFX
  //   oled.setTextSize(2);
  //   oled.setCursor(10, 28);
  //   oled.print("[No Projects]");
  //   oled.display();
  //   return;
  // }
  //
  // // --- Draw Project Name with Bold Font ---
  // oled.setFont(&FreeSansBold9pt7b); // Use bold font
  // oled.setTextSize(1);              // Size 1 for this font is good
  // String name = projects[selectedIndex].name;
  //
  // // Truncation Logic
  // int16_t x1, y1;
  // uint16_t w, h;
  // oled.getTextBounds(name, 0, 0, &x1, &y1, &w, &h);
  // int maxWidth = oled.width() - 8; // Slightly more margin for this font
  // if (w > maxWidth)
  // {
  //   int maxChars = (maxWidth / (w / name.length())) - 2;
  //   if (maxChars < 1)
  //     maxChars = 1;
  //   name = name.substring(0, maxChars) + "...";
  //   oled.getTextBounds(name, 0, 0, &x1, &y1, &w, &h);
  // }
  //
  // // Center the text horizontally and vertically below the title box
  // int16_t x = (oled.width() - w) / 2;
  // int16_t titleBoxBottom = boxY + th + boxPaddingY_Top + boxPaddingY_Bottom + 1;
  // int16_t y = titleBoxBottom + ((oled.height() - titleBoxBottom - 12) / 2) + 8; // Adjusted to make room for pagination dots
  //
  // oled.setCursor(x, y);
  // oled.print(name);
  //
  // // --- Draw Pagination Dots ---
  // if (projects.size() > 1)
  // {
  //   // Calculate total width of all dots and spacing
  //   const int dotRadius = 2;
  //   const int dotSpacing = 4;
  //   const int dotDiameter = dotRadius * 2;
  //   const int totalWidth = (projects.size() * dotDiameter) + ((projects.size() - 1) * dotSpacing);
  //
  //   // Calculate starting X position to center the dots
  //   const int dotsStartX = (oled.width() - totalWidth) / 2;
  //   const int dotsY = oled.height() - 7; // 7 pixels from bottom
  //
  //   // Draw all dots
  //   for (int i = 0; i < projects.size(); i++)
  //   {
  //     int dotX = dotsStartX + (i * (dotDiameter + dotSpacing));
  //
  //     if (i == selectedIndex)
  //     {
  //       // Selected dot (filled)
  //       oled.fillCircle(dotX + dotRadius, dotsY, dotRadius, SSD1306_WHITE);
  //     }
  //     else
  //     {
  //       // Unselected dot (outline)
  //       oled.drawCircle(dotX + dotRadius, dotsY, dotRadius, SSD1306_WHITE);
  //     }
  //   }
  // }
  //
  // // Reset font for other screens
  // oled.setFont();
  //
  // oled.display();
}

