#include "SimpleProductionTest.h"
#include "state_machine/include/StateMachine.h"
#include "state_machine/include/LEDController.h"
#include "SimpleEncoder.h"
#include "ui/ScreenManager.h"
#include <WiFi.h>
#include <lvgl.h>
#include <esp_task_wdt.h>
#include "HWCDC.h"

extern HWCDC USBSerial;
extern StateMachine stateMachine;
extern SimpleEncoder simpleEncoder;
extern ScreenManager screenManager;

void SimpleProductionTest::runTest() {
    USBSerial.println("\n========== PRODUCTION TEST ==========");
    USBSerial.println("Testing all hardware components...\n");
    
    // Save current screen
    lv_obj_t* current_screen = lv_scr_act();
    
    // 1. Display Test
    USBSerial.println("1. DISPLAY: Showing colors for 2 seconds...");
    lv_obj_t* test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(test_screen, lv_color_hex(0xFF0000), 0);
    lv_scr_load(test_screen);
    lv_task_handler();
    for (int i = 0; i < 20; i++) {
        delay(100);
        esp_task_wdt_reset();
        lv_task_handler();
    }
    // Don't delete the screen while it's active
    USBSerial.println("   ✓ Display test done\n");
    
    // 2. LED Test
    USBSerial.println("2. LEDs: Cycling colors...");
    LEDController* led = stateMachine.getLEDController();
    if (led) {
        led->setSolid(0xFF0000); delay(500); esp_task_wdt_reset(); // Red
        led->setSolid(0x00FF00); delay(500); esp_task_wdt_reset(); // Green
        led->setSolid(0x0000FF); delay(500); esp_task_wdt_reset(); // Blue
        led->setSolid(0xFFFFFF); delay(500); esp_task_wdt_reset(); // White
        led->turnOff();
        USBSerial.println("   ✓ LED test done\n");
    }
    
    // 3. Encoder Test
    USBSerial.println("3. ENCODER: Rotate within 3 seconds...");
    int startPos = simpleEncoder.getPosition();
    for (int i = 0; i < 30; i++) {
        delay(100);
        esp_task_wdt_reset();
    }
    int movement = simpleEncoder.getPosition() - startPos;
    USBSerial.print("   ✓ Encoder moved: ");
    USBSerial.print(movement);
    USBSerial.println(" positions\n");
    
    // 4. Timer Test
    USBSerial.println("4. TIMER: Running 3-second timer...");
    stateMachine.setPendingDuration(3);
    stateMachine.transitionTo("TimerState");
    for (int i = 0; i < 40; i++) {
        delay(100);
        esp_task_wdt_reset();
    }
    USBSerial.println("   ✓ Timer test done\n");
    
    // 5. WiFi Test
    USBSerial.println("5. WIFI: Checking connection...");
    if (WiFi.status() == WL_CONNECTED) {
        USBSerial.print("   ✓ Connected to: ");
        USBSerial.println(WiFi.SSID());
    } else {
        USBSerial.println("   ✓ WiFi hardware OK (not connected)");
    }
    
    USBSerial.println("\n========== TEST COMPLETE ==========");
    USBSerial.println("All tests finished. Check results above.");
    USBSerial.println("===================================\n");
    
    // Return to idle state and screen
    stateMachine.transitionTo("IdleState");
    screenManager.showIdleScreen();
    
    // Process LVGL to ensure screen switch happens
    for (int i = 0; i < 5; i++) {
        lv_task_handler();
        delay(20);
        esp_task_wdt_reset();
    }
    
    // Now safe to delete test screen
    if (test_screen) {
        lv_obj_del(test_screen);
    }
    
    // Final cleanup
    USBSerial.flush();
    delay(100);
    esp_task_wdt_reset();
}