#include "controllers/DisplayController.h"

#include "fonts/Picopixel.h"
#include "fonts/Org_01.h"
#include "bitmaps.h"
#include <Fonts/FreeSansBold9pt7b.h>

DisplayController::DisplayController(uint8_t oledWidth, uint8_t oledHeight, uint8_t oledAddress)
    : oled(oledWidth, oledHeight, &Wire, -1), animation(&oled) {}

void DisplayController::begin()
{
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Loop forever if initialization fails
  }

  // oled.ssd1306_command(SSD1306_SETCONTRAST);
  // oled.ssd1306_command(128);
  // Set contrast to maximum (0xFF = 255) for potentially brighter display
  oled.ssd1306_command(SSD1306_SETCONTRAST);
  oled.ssd1306_command(0xFF); // Use 0xFF (255) for max contrast/brightness

  oled.clearDisplay();
  oled.display();
  Serial.println("DisplayController initialized.");
}

void DisplayController::drawSplashScreen()
{
  oled.clearDisplay();

  oled.drawBitmap(16, 3, focusdial_logo, 99, 45, 1);
  oled.setTextColor(1);
  oled.setTextSize(1);
  oled.setFont(&Picopixel);
  oled.setCursor(21, 60);
  oled.print("YOUTUBE/ @SALIMBENBOUZ");

  oled.display();
}

void DisplayController::drawIdleScreen(int durationMinutes, bool wifi)
{
  if (isAnimationRunning())
    return;

  // Restore original blinking logic for WiFi icon when disconnected
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = true;
  unsigned long currentTime = millis();
  if (!wifi && (currentTime - lastBlinkTime >= 500))
  {
    blinkState = !blinkState;
    lastBlinkTime = currentTime;
  }

  oled.clearDisplay();

  // Restore original "PRESS TO START" label
  oled.setFont(&Picopixel);
  oled.setTextSize(1);
  oled.setTextColor(1);
  oled.setCursor(40, 58);
  oled.print("PRESS TO START");
  oled.drawRoundRect(35, 51, 60, 11, 1, 1);

  // Restore original WiFi icon display logic (top right with text)
  if (wifi)
  {
    oled.drawBitmap(70, 3, icon_wifi_on, 5, 5, 1);
    oled.setCursor(54, 7);
    oled.print("WIFI");
  }
  else if (blinkState) // Only draw 'off' icon when blinking
  {
    oled.drawBitmap(70, 3, icon_wifi_off, 5, 5, 1);
    oled.setCursor(54, 7);
    oled.print("WIFI");
  }

  // Check if duration is indeterminate (0)
  if (durationMinutes == 0)
  {
    // Draw infinity icon centered, replacing the time digits ONLY
    int iconWidth = 48;
    int iconHeight = 24;
    int x = (oled.width() - iconWidth) / 2;
    int y = (oled.height() - iconHeight) / 2; // Approximately center vertically
    oled.drawBitmap(x, y, icon_infinity, iconWidth, iconHeight, 1);
    // NO H/M/S labels, NO separator dots for infinity icon
  }
  else
  {
    // Restore original logic to display MM:00 using large font
    char left[3], right[3];
    int xLeft = 1;
    int xRight = 73;

    // Idle screen always shows MM:00 format based on durationMinutes
    sprintf(left, "%02d", durationMinutes);
    strcpy(right, "00");

    // Adjust X position if the first character is '1'
    if (left[0] == '1')
    {
      xLeft += 20;
    }
    // Right side is always "00", no need to check '1'

    oled.setTextSize(5);
    oled.setFont(&Org_01);
    oled.setCursor(xLeft, 36);
    oled.print(left);

    oled.setCursor(xRight, 36);
    oled.print(right);

    // Restore original separator dots
    oled.fillRect(62, 21, 5, 5, 1);
    oled.fillRect(62, 31, 5, 5, 1);

    // REMOVE any H/M/S labels below the time - ensure they are not drawn
    // oled.setTextSize(1);
    // oled.setFont(&Picopixel);
    // oled.setCursor(27, 54);
    // oled.print("M");
    // oled.setCursor(98, 54);
    // oled.print("S");
  }

  oled.display();
}

void DisplayController::drawTimerScreen(int timeValue, bool isCountUp)
{
  if (isAnimationRunning())
    return;

  oled.clearDisplay();

  int displaySeconds = timeValue;
  if (!isCountUp && displaySeconds < 0) // Ensure remainingSeconds doesn't go below 0 for countdown display
  {
    displaySeconds = 0;
  }
  else if (isCountUp && displaySeconds < 0) // Should not happen for elapsed, but safety check
  {
    displaySeconds = 0;
  }

  // Calculate H, M, S based on the time value (which is either elapsed or remaining)
  int hours = displaySeconds / 3600;
  int minutes = (displaySeconds % 3600) / 60;
  int seconds = displaySeconds % 60;

  char left[3], right[3];
  int xLeft = 1;
  int xRight = 73;
  int yPos = 36; // Default Y position for HH:MM

  // Format left and right (HH:MM or MM:SS)
  if (hours > 0 || (isCountUp && displaySeconds >= 3600)) // Show HH:MM if hours > 0 OR if counting up and reached an hour
  {
    sprintf(left, "%02d", hours);
    sprintf(right, "%02d", minutes);
    // yPos remains 36
  }
  else // Show MM:SS otherwise
  {
    sprintf(left, "%02d", minutes);
    sprintf(right, "%02d", seconds);
    yPos = 40; // Adjust Y position for MM:SS to center vertically more
  }

  // Adjust X position if the first character is '1'
  if (left[0] == '1')
  {
    xLeft += 20;
  }
  if (right[0] == '1')
  {
    xRight += 20;
  }

  // Draw the large digits
  oled.setTextColor(1);
  oled.setTextSize(5);
  oled.setFont(&Org_01);
  oled.setCursor(xLeft, yPos);
  oled.print(left);
  oled.setCursor(xRight, yPos);
  oled.print(right);

  // Draw separator dots
  oled.fillRect(62, yPos - 15, 5, 5, 1); // Upper dot
  oled.fillRect(62, yPos - 5, 5, 5, 1);  // Lower dot

  // Draw labels (H/M or M/S)
  oled.setTextSize(1);
  oled.setFont(&Picopixel);
  if (hours > 0 || (isCountUp && displaySeconds >= 3600)) // Show H and M if hours > 0 OR if counting up and reached an hour
  {
    oled.setCursor(27, 54);
    oled.print("H");
    oled.setCursor(98, 54);
    oled.print("M");
    // Optionally draw a small indicator for count-up mode?
    // if (isCountUp) { oled.drawBitmap(61, 3, icon_up_arrow, 7, 7, 1); }
  }
  else // Show M and S otherwise
  {
    oled.setCursor(27, 54);
    oled.print("M");
    oled.setCursor(98, 54);
    oled.print("S");
    // Optionally draw a small indicator for count-up mode?
    // if (isCountUp) { oled.drawBitmap(61, 3, icon_up_arrow, 7, 7, 1); }
  }

  oled.display();
}

void DisplayController::drawPausedScreen(int remainingSeconds)
{
  if (isAnimationRunning())
    return;

  oled.clearDisplay();

  if (remainingSeconds < 0)
  {
    remainingSeconds = 0;
  }

  int hours = remainingSeconds / 3600;
  int minutes = (remainingSeconds % 3600) / 60;
  int seconds = remainingSeconds % 60;

  char left[3], right[3];
  int xLeft = 1;
  int xRight = 73;

  // Format left and right
  if (hours > 0)
  {
    sprintf(left, "%02d", hours);
    sprintf(right, "%02d", minutes);
  }
  else
  {
    sprintf(left, "%02d", minutes);
    sprintf(right, "%02d", seconds);
  }

  // Adjust position if the first character is '1'
  if (left[0] == '1')
  {
    xLeft += 20;
  }
  if (right[0] == '1')
  {
    xRight += 20;
  }

  if ((millis() / 400) % 2 == 0)
  {
    oled.setTextColor(1);
    oled.setTextSize(5);
    oled.setFont(&Org_01);
    oled.setCursor(xLeft, 36);
    oled.print(left);
    oled.setCursor(xRight, 36);
    oled.print(right);

    oled.fillRect(62, 31, 5, 5, 1);
    oled.fillRect(62, 22, 5, 5, 1);

    oled.setFont(&Org_01);
    oled.setTextSize(1);
    oled.setCursor(27, 54);
    oled.print(hours > 0 ? "H" : "M");
    oled.setCursor(98, 54);
    oled.print(hours > 0 ? "M" : "S");
  }

  // Draw label and icon
  oled.drawRoundRect(47, 51, 35, 11, 1, 1);
  oled.setTextColor(1);
  oled.setTextSize(1);
  oled.setFont(&Picopixel);
  oled.setCursor(53, 58);
  oled.print("PAUSED");
  oled.drawBitmap(60, 2, icon_pause, 9, 9, 1);

  oled.display();
}

void DisplayController::drawResetScreen(bool resetSelected)
{
  if (isAnimationRunning())
    return;
  oled.clearDisplay();

  // Static UI elements
  oled.setTextColor(1);
  oled.setTextSize(2);
  oled.setFont(&Picopixel);
  oled.setCursor(54, 15);
  oled.print("RESET");
  oled.setTextSize(1);
  oled.setCursor(20, 30);
  oled.print("ALL STORED SETTINGS WILL ");
  oled.setCursor(21, 40);
  oled.print("BE PERMANENTLY ERASED.");
  oled.drawBitmap(35, 4, icon_reset, 13, 16, 1);

  // Change only the rectangle fill and text color based on selection
  if (resetSelected)
  {
    // "RESET" filled, "CANCEL" outlined
    oled.fillRoundRect(67, 49, 37, 11, 1, 1);
    oled.setTextColor(0);
    oled.setCursor(76, 56);
    oled.print("RESET");

    oled.drawRoundRect(24, 49, 37, 11, 1, 1);
    oled.setTextColor(1);
    oled.setCursor(31, 56);
    oled.print("CANCEL");
  }
  else
  {
    // "CANCEL" filled, "RESET" outlined
    oled.fillRoundRect(24, 49, 37, 11, 1, 1);
    oled.setTextColor(0);
    oled.setCursor(31, 56);
    oled.print("CANCEL");

    oled.drawRoundRect(67, 49, 37, 11, 1, 1);
    oled.setTextColor(1);
    oled.setCursor(76, 56);
    oled.print("RESET");
  }

  oled.display();
}

void DisplayController::drawDoneScreen(unsigned long finalElapsedTime)
{
  if (isAnimationRunning())
    return;

  oled.clearDisplay();

  // Calculate H, M, S from finalElapsedTime
  int hours = finalElapsedTime / 3600;
  int minutes = (finalElapsedTime % 3600) / 60;
  int seconds = finalElapsedTime % 60;

  char left[3], right[3];
  int xLeft = 1;
  int xRight = 73;
  int yPos = 36;

  // Format left and right (HH:MM or MM:SS)
  if (hours > 0) // Show HH:MM if duration was an hour or more
  {
    sprintf(left, "%02d", hours);
    sprintf(right, "%02d", minutes);
  }
  else // Show MM:SS otherwise
  {
    sprintf(left, "%02d", minutes);
    sprintf(right, "%02d", seconds);
    yPos = 40; // Adjust Y slightly for MM:SS
  }

  // Adjust X position if the first character is '1'
  if (left[0] == '1')
  {
    xLeft += 20;
  }
  if (right[0] == '1')
  {
    xRight += 20;
  }

  // Draw the large digits
  oled.setTextColor(1);
  oled.setTextSize(5);
  oled.setFont(&Org_01);
  oled.setCursor(xLeft, yPos);
  oled.print(left);
  oled.setCursor(xRight, yPos);
  oled.print(right);

  // Draw separator dots
  oled.fillRect(62, yPos - 15, 5, 5, 1); // Upper dot
  oled.fillRect(62, yPos - 5, 5, 5, 1);  // Lower dot

  // Draw labels (H/M or M/S)
  oled.setTextSize(1);
  oled.setFont(&Picopixel);
  if (hours > 0)
  {
    oled.setCursor(27, 54);
    oled.print("H");
    oled.setCursor(98, 54);
    oled.print("M");
  }
  else
  {
    oled.setCursor(27, 54);
    oled.print("M");
    oled.setCursor(98, 54);
    oled.print("S");
  }

  // Draw label and icon (Keep "DONE" label)
  oled.fillRoundRect(46, 51, 35, 11, 1, 1);
  oled.setTextColor(0); // Text inside box is black (inverted)
  oled.setTextSize(1);
  oled.setFont(&Picopixel);
  oled.setCursor(56, 58);
  oled.print("DONE");
  // oled.drawBitmap(61, 3, icon_star, 7, 7, 1); // Remove star, keep it clean

  oled.display();
}

void DisplayController::drawAdjustScreen(int duration, bool wifi)
{
  if (isAnimationRunning())
    return;

  oled.clearDisplay();

  // Restore original "PRESS TO SAVE" label
  oled.setFont(&Picopixel);
  oled.setTextSize(1);
  oled.setTextColor(1);
  oled.setCursor(40, 58);
  oled.print("PRESS TO SAVE");
  oled.drawRoundRect(35, 51, 60, 11, 1, 1);

  // Restore original WiFi icon display logic (top right with text)
  oled.drawBitmap(70, 3, wifi ? icon_wifi_on : icon_wifi_off, 5, 5, 1);
  oled.setCursor(54, 7);
  oled.print("WIFI");

  // Check for indeterminate mode (duration == 0)
  if (duration == 0)
  {
    // Display Infinity Icon for indeterminate mode, replacing time digits/dashes ONLY
    const int iconWidth = 48;
    const int iconHeight = 24;
    const int iconX = (128 - iconWidth) / 2; // Center horizontally
    const int iconY = 18;                    // Position vertically (like original digits)
    oled.drawBitmap(iconX, iconY, icon_infinity, iconWidth, iconHeight, 1);
    // NO H/M/S labels, NO separator dots for infinity icon
  }
  else
  {
    // Restore original logic for HH:MM or MM:00 display
    char left[3], right[3];
    int xLeft = 1;
    int xRight = 73;

    // Determine format based on original logic (assuming MM:00 for <60, HH:MM for >=60)
    // CHANGE: Always use HH:MM format for consistency
    int hours = duration / 60;
    int minutes = duration % 60;
    sprintf(left, "%02d", hours);
    sprintf(right, "%02d", minutes);

    // Adjust X position if the first character is '1'
    if (left[0] == '1')
    {
      xLeft += 20;
    }
    if (right[0] == '1')
    {
      xRight += 20;
    }

    oled.setTextSize(5);
    oled.setFont(&Org_01);
    oled.setCursor(xLeft, 36);
    oled.print(left);

    oled.setCursor(xRight, 36);
    oled.print(right);

    // Restore original separator dots
    oled.fillRect(62, 21, 5, 5, 1);
    oled.fillRect(62, 31, 5, 5, 1);

    // REMOVE any H/M/S labels below the time - ensure they are not drawn
    // oled.setTextSize(1);
    // oled.setFont(&Picopixel);
    // if (duration >= 60) { ... } else { ... }
  }

  oled.display();
}

void DisplayController::drawProvisionScreen()
{
  if (isAnimationRunning())
    return;

  oled.clearDisplay();

  oled.setTextColor(1);
  oled.setTextSize(1);
  oled.setFont(&Picopixel);
  oled.setCursor(12, 38);
  oled.print("PLEASE CONNECT TO BLUETOOTH");
  oled.setCursor(14, 48);
  oled.print("AND THIS FOCUSDIAL NETWORK");
  oled.setCursor(35, 58);
  oled.print("TO PROVISION WIFI");
  oled.drawBitmap(39, 4, provision_logo, 51, 23, 1);

  oled.display();
}

void DisplayController::clear()
{
  oled.clearDisplay();
  oled.display();
}

void DisplayController::showAnimation(const byte frames[][288], int frameCount, bool loop, bool reverse, unsigned long durationMs, int width, int height)
{
  animation.start(&frames[0][0], frameCount, loop, reverse, durationMs, width, height); // Pass array as pointer
}

void DisplayController::updateAnimation()
{
  animation.update();
}

bool DisplayController::isAnimationRunning()
{
  return animation.isRunning();
}

void DisplayController::showConfirmation()
{
  showAnimation(animation_tick, 20);
}

void DisplayController::showCancel()
{
  showAnimation(animation_cancel, 18, false, true);
}

void DisplayController::showReset()
{
  showAnimation(animation_reset, 28, true, false);
}

void DisplayController::showConnected()
{
  showAnimation(animation_wifi, 28);
}

void DisplayController::showTimerStart()
{
  showAnimation(animation_timer_start, 20, false, true);
}

void DisplayController::showTimerDone()
{
  showAnimation(animation_timer_start, 20);
}

void DisplayController::showTimerPause()
{
  showAnimation(animation_resume, 18, false, true);
}

void DisplayController::showTimerResume()
{
  showAnimation(animation_resume, 18);
}

// Draw the project selection screen - Title in box, centered name with bold font
void DisplayController::drawProjectSelectionScreen(const ProjectList &projects, int selectedIndex, int topIndex, int numToShow)
{
  if (isAnimationRunning())
    return;

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextWrap(false);

  // --- Draw Title in a Box ---
  oled.setFont(&Picopixel);
  oled.setTextSize(1);
  const char *title = "SELECT PROJECT";
  int16_t tx1, ty1, titleX, titleY;
  uint16_t tw, th;
  oled.getTextBounds(title, 0, 0, &tx1, &ty1, &tw, &th);
  titleX = (oled.width() - tw) / 2; // Center horizontally
  titleY = 8;                       // Set Y position for text baseline
  oled.setCursor(titleX, titleY);
  oled.print(title);
  // Draw rounded box around title
  int boxPaddingX = 3;
  int boxPaddingY_Top = 2;
  int boxPaddingY_Bottom = 3;               // Increase bottom padding
  int boxY = titleY - th - boxPaddingY_Top; // Box top Y
  oled.drawRoundRect(titleX - boxPaddingX, boxY, tw + (2 * boxPaddingX), th + boxPaddingY_Top + boxPaddingY_Bottom + 1, 1, SSD1306_WHITE);

  // Check if the selected index is valid
  if (selectedIndex < 0 || selectedIndex >= projects.size())
  {
    oled.setFont(); // Reset to default GFX
    oled.setTextSize(2);
    oled.setCursor(10, 28);
    oled.print("[No Projects]");
    oled.display();
    return;
  }

  // --- Draw Project Name with Bold Font ---
  oled.setFont(&FreeSansBold9pt7b); // Use bold font
  oled.setTextSize(1);              // Size 1 for this font is good
  String name = projects[selectedIndex].name;

  // Truncation Logic
  int16_t x1, y1;
  uint16_t w, h;
  oled.getTextBounds(name, 0, 0, &x1, &y1, &w, &h);
  int maxWidth = oled.width() - 8; // Slightly more margin for this font
  if (w > maxWidth)
  {
    int maxChars = (maxWidth / (w / name.length())) - 2;
    if (maxChars < 1)
      maxChars = 1;
    name = name.substring(0, maxChars) + "...";
    oled.getTextBounds(name, 0, 0, &x1, &y1, &w, &h);
  }

  // Center the text horizontally and vertically below the title box
  int16_t x = (oled.width() - w) / 2;
  int16_t titleBoxBottom = boxY + th + boxPaddingY_Top + boxPaddingY_Bottom + 1;
  int16_t y = titleBoxBottom + ((oled.height() - titleBoxBottom - 12) / 2) + 8; // Adjusted to make room for pagination dots

  oled.setCursor(x, y);
  oled.print(name);

  // --- Draw Pagination Dots ---
  if (projects.size() > 1)
  {
    // Calculate total width of all dots and spacing
    const int dotRadius = 2;
    const int dotSpacing = 4;
    const int dotDiameter = dotRadius * 2;
    const int totalWidth = (projects.size() * dotDiameter) + ((projects.size() - 1) * dotSpacing);

    // Calculate starting X position to center the dots
    const int dotsStartX = (oled.width() - totalWidth) / 2;
    const int dotsY = oled.height() - 7; // 7 pixels from bottom

    // Draw all dots
    for (int i = 0; i < projects.size(); i++)
    {
      int dotX = dotsStartX + (i * (dotDiameter + dotSpacing));

      if (i == selectedIndex)
      {
        // Selected dot (filled)
        oled.fillCircle(dotX + dotRadius, dotsY, dotRadius, SSD1306_WHITE);
      }
      else
      {
        // Unselected dot (outline)
        oled.drawCircle(dotX + dotRadius, dotsY, dotRadius, SSD1306_WHITE);
      }
    }
  }

  // Reset font for other screens
  oled.setFont();

  oled.display();
}

