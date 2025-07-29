/**
 * Timer Arduino - Smooth Scrolling Demo
 * Three black screens with centered text
 * For ESP32-S3-Touch-AMOLED-1.75
 */

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "HWCDC.h"
#include "XPowersLib.h"
// #include <ESP_IOExpander_Library.h> // Disabled due to I2C driver conflict
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WebServer.h>
// #include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPmDNS.h>

// State Machine
#include "src/state_machine/include/StateMachine.h"
#include "src/state_machine/include/LEDController.h"
#include "src/state_machine/states/TimerState.h"
#include "src/state_machine/states/ProjectSelectState.h"

// UI
#include "src/ui/ScreenManager.h"
#include "src/ui/UIEventHandler.h"
#include "src/ui/TouchManager.h"
#include "src/ProjectManager.h"

// Simple Encoder
#include "src/SimpleEncoder.h"


// Declare custom fonts
LV_FONT_DECLARE(lv_font_roboto_mono_120);
LV_FONT_DECLARE(lv_font_barlow_24);
LV_FONT_DECLARE(lv_font_barlow_bold_48);

// Color definitions for consistent styling
#define COLOR_FOREGROUND lv_color_hex(0xDDDDDD)  // Main text color - brightest
#define COLOR_BACKGROUND lv_color_hex(0x888888)  // Secondary elements - much dimmer

// USB Serial for ESP32-S3
HWCDC USBSerial;

// Power Management
XPowersAXP2101 power;

// Power button management
bool pmu_flag = false;
// Sleep functionality disabled - was causing stack overflow
// unsigned long last_activity_time = 0;
// const unsigned long INACTIVITY_TIMEOUT = 180000; // 3 minutes
bool is_sleeping = false;  // Keep for now to avoid breaking other code
// bool is_waking_up = false;
// unsigned long wake_time = 0;
// const unsigned long MIN_AWAKE_TIME = 2000;

// Wake-up configuration
#define WAKE_BUTTON_PIN GPIO_NUM_0  // BOOT button as wake source
#define PMU_IRQ_PIN GPIO_NUM_6      // AXP2101 IRQ pin (potentially direct connection)
RTC_DATA_ATTR int bootCount = 0;   // RTC memory variable to track boot count

// Display Configuration
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 39
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Touch Configuration
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 40

// LVGL Configuration
#define LVGL_TICK_PERIOD_MS 2  // 2ms as per working example

// Display objects
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1,
    LCD_SDIO2, LCD_SDIO3);

Arduino_GFX *gfx = new Arduino_CO5300(
    bus,
    LCD_RESET,
    0,     // rotation
    false, // IPS
    LCD_WIDTH,
    LCD_HEIGHT,
    6,     // col_offset1
    0,     // row_offset1
    0,     // col_offset2
    0      // row_offset2
);

// LVGL variables
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// Screen objects
static lv_obj_t *screen1;
static lv_obj_t *screen2;
static lv_obj_t *screen3;
static lv_obj_t *label1;
static lv_obj_t *label2;
static lv_obj_t *label3;

// Timer display elements
static lv_obj_t *minutes_label;
static lv_obj_t *seconds_label;
static lv_obj_t *date_label;

// Screen management
static uint8_t current_screen = 0;
static bool transitioning = false;

// Animation types - simplified to fade only
typedef enum {
    TRANS_FADE
} transition_type_t;

// Global screen manager instance
ScreenManager screenManager;
TouchManager touchManager;

// Touch handling
#include "TouchDrvCSTXXX.hpp"
TouchDrvCSTXXX touch;
int16_t touch_x[5], touch_y[5];

// Simple debounce for touch
unsigned long lastTouchTime = 0;
const unsigned long TOUCH_DEBOUNCE_MS = 50; // 50ms debounce

// Global input device handle for touch management
lv_indev_t * touch_indev = NULL;

// Track if we're in a touch sequence
bool touchSequenceActive = false;
unsigned long touchSequenceStartTime = 0;

// Forward declarations
void go_to_next_screen();
void go_to_prev_screen();
void setPMUFlag(void);
void goToSleep(bool deep_sleep = false);
void resetActivityTimer();
void handleWakeUp();

// Web Server
WebServer apiServer(80);
// WebSocketsServer webSocket(81);
bool webServerRunning = false;

void startWebServer();
void handleApiProjects();
void handleApiStatus();
void handleApiProjectsPost();
void handleApiUpdateProject();
void handleApiDeleteProject();
void handleApiWebhookGet();
void handleApiWebhookPost();
void handleApiKeyGet();
void handleApiKeyPost();
void handleNotFound();
// void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


// Rounder callback for display optimization (critical for performance)
void display_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area) {
    // Ensure x1 and y1 are even
    if(area->x1 % 2 != 0) area->x1--;
    if(area->y1 % 2 != 0) area->y1--;
    // Ensure x2 and y2 are odd
    if(area->x2 % 2 == 0) area->x2++;
    if(area->y2 % 2 == 0) area->y2++;
}

// Display flush callback
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(disp);
}

// Touch read callback - now uses TouchManager for release-only processing
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    static int16_t last_x = 0, last_y = 0;
    unsigned long now = millis();
    
    // Simple debounce
    if (now - lastTouchTime < TOUCH_DEBOUNCE_MS) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    
    // Read touch directly
    uint8_t touch_count = touch.getPoint(touch_x, touch_y, touch.getSupportTouchPoint());
    
    bool touchDetected = (touch_count > 0);
    if (touchDetected) {
        last_x = touch_x[0];
        last_y = touch_y[0];
        lastTouchTime = now;
    }
    
    // Process touch through TouchManager (handles all logic)
    touchManager.processTouch(touchDetected, last_x, last_y);
    
    // Always report release to LVGL - we don't want LVGL processing any events
    data->state = LV_INDEV_STATE_REL;
    data->point.x = last_x;
    data->point.y = last_y;
}

// LVGL tick function
void lv_tick_task(void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// Screen transition with fade animation only
void transition_to_screen(lv_obj_t *new_screen, uint32_t duration) {
    if (transitioning) return;  // Prevent overlapping transitions
    
    transitioning = true;
    
    USBSerial.println("Transitioning to screen with fade animation");
    
    lv_scr_load_anim(new_screen, LV_SCR_LOAD_ANIM_FADE_IN, duration, 0, false);
    
    // Reset transition flag after animation completes
    lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
        transitioning = false;
        lv_timer_del(t);
    }, duration + 50, NULL);
}


// Navigate to next screen
void go_to_next_screen() {
    current_screen = (current_screen + 1) % 3;
    lv_obj_t *target = (current_screen == 0) ? screen1 : (current_screen == 1) ? screen2 : screen3;
    transition_to_screen(target, 500);  // 500ms fade duration
}

// Navigate to previous screen
void go_to_prev_screen() {
    current_screen = (current_screen + 2) % 3;  // +2 is same as -1 with wrap
    lv_obj_t *target = (current_screen == 0) ? screen1 : (current_screen == 1) ? screen2 : screen3;
    transition_to_screen(target, 500);  // 500ms fade duration
}


// Create the screens using the screen manager
void create_screens() {
    // Initialize the screen manager
    screenManager.init();
}

// PMU interrupt flag setter
void setPMUFlag(void) {
    pmu_flag = true;
}

// Reset the inactivity timer - DISABLED
void resetActivityTimer() {
    // Function disabled - was causing stack overflow
}

// Enter sleep mode - DISABLED
void goToSleep(bool deep_sleep) {
    // Function disabled - was causing issues
    return;
    
    if (deep_sleep) {
        USBSerial.println("Entering DEEP sleep mode (power button pressed)...");
    } else {
        USBSerial.println("Entering light sleep mode (inactivity)...");
    }
    USBSerial.flush();
    delay(10); // Allow serial to flush
    
    is_sleeping = true;
    
    // Turn off display backlight gradually
    for (int i = 200; i >= 0; i -= 5) {
        gfx->Display_Brightness(i);
        delay(10);
    }
    
    if (deep_sleep) {
        // Deep sleep configuration - only BOOT button can wake
        // Configure wake-up source - BOOT button only
        esp_sleep_enable_ext0_wakeup(WAKE_BUTTON_PIN, 0); // Wake on LOW
        
        // Note: Touch wake NOT enabled for deep sleep
        USBSerial.println("Deep sleep: Only BOOT button can wake!");
        USBSerial.flush();
        delay(10);
        
        // Enter deep sleep - ESP32 will restart on wake
        esp_deep_sleep_start();
        // Code never reaches here
    } else {
        // Light sleep configuration - ONLY touch wake enabled
        // Clear any PMU interrupts
        power.clearIrqStatus();
        
        // Configure wake-up sources for light sleep
        // Touch pin wake-up ONLY
        gpio_wakeup_enable(GPIO_NUM_11, GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        
        USBSerial.println("Light sleep: Touch screen to wake!");
        USBSerial.flush();
        delay(10);
        
        // Enter light sleep (keeps RAM, faster wake-up)
        esp_light_sleep_start();
        
        // Code resumes here after wake-up
        handleWakeUp();
    }
}

// Handle wake-up from sleep - DISABLED
void handleWakeUp() {
    // Function disabled
    return;
    
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    USBSerial.println("Waking up from light sleep...");
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_GPIO:
            USBSerial.println("Wake-up from touch screen");
            break;
        default:
            USBSerial.println("Wake-up from unknown source");
            break;
    }
    
    // Clear any pending interrupts
    power.clearIrqStatus();
    
    // Restore display brightness
    for (int i = 0; i <= 200; i += 5) {
        gfx->Display_Brightness(i);
        delay(10);
    }
    
    // Timer reset disabled
    unsigned long current = millis();
    // last_activity_time = current;
    // wake_time = current;
    
    // Restore normal PMU interrupt configuration
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    power.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | 
                   XPOWERS_AXP2101_VBUS_INSERT_IRQ | 
                   XPOWERS_AXP2101_VBUS_REMOVE_IRQ |
                   XPOWERS_AXP2101_BAT_CHG_START_IRQ |
                   XPOWERS_AXP2101_BAT_CHG_DONE_IRQ);
    
    USBSerial.print("System resumed from light sleep - timers reset to: ");
    USBSerial.println(current);
}

void setup() {
    USBSerial.begin(115200);
    // USBSerial.setDebugOutput(true);
    // while(!USBSerial);
    USBSerial.println("Timer Arduino - Starting");
    USBSerial.println("==== FONT DEBUG INFO ====");
    
    // Check wake-up reason at startup
    ++bootCount;
    USBSerial.print("Boot count: ");
    USBSerial.println(bootCount);
    
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED) {
        USBSerial.print("Wake-up reason at boot: ");
        switch(wakeup_reason) {
            case ESP_SLEEP_WAKEUP_EXT0:
                USBSerial.println("External GPIO (BOOT button)");
                break;
            case ESP_SLEEP_WAKEUP_GPIO:
                USBSerial.println("GPIO wake-up");
                break;
            case ESP_SLEEP_WAKEUP_TIMER:
                USBSerial.println("Timer wake-up");
                break;
            default:
                USBSerial.println("Other");
                break;
        }
    }

    // Initialize I2C for touch and PMU
    Wire.begin(IIC_SDA, IIC_SCL);
    
    // IO Expander disabled due to I2C driver conflict
    // Will use polling method for PMU interrupts instead
    
    // Initialize AXP2101 PMU
    USBSerial.println("Initializing PMU...");
    delay(100); // Give PMU time to wake up
    
    bool pmu_result = power.begin(Wire, 0x34, IIC_SDA, IIC_SCL);
    if (pmu_result == false) {
        USBSerial.println("PMU initialization failed, retrying...");
        delay(500);
        pmu_result = power.begin(Wire, 0x34, IIC_SDA, IIC_SCL);
    }
    
    if (pmu_result == false) {
        USBSerial.println("PMU is not online... continuing without power button support");
    } else {
        USBSerial.println("PMU initialized successfully");
        
        // Configure PMU
        power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
        power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
        power.clearIrqStatus();
        
        // Enable power button interrupt and battery status interrupts
        power.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | 
                       XPOWERS_AXP2101_VBUS_INSERT_IRQ | 
                       XPOWERS_AXP2101_VBUS_REMOVE_IRQ |
                       XPOWERS_AXP2101_BAT_CHG_START_IRQ |
                       XPOWERS_AXP2101_BAT_CHG_DONE_IRQ);
        
        // Enable wake-up functionality
        power.enableWakeup();
        power.enableFastWakeup();
        
        // Configure wake-up control
        // Enable wake on power button press (IRQ pin goes low on button press)
        power.wakeupControl(XPOWERS_AXP2101_WAKEUP_IRQ_PIN_TO_LOW, true);
        
        // Configure IRQ output behavior
        // Set IRQ to output when power button is pressed
        power.setIrqLevel(0); // 0 = active low, 1 = active high
        
        // Enable ADC measurements
        power.enableTemperatureMeasure();
        power.enableBattDetection();
        power.enableVbusVoltageMeasure();
        power.enableBattVoltageMeasure();
        power.enableSystemVoltageMeasure();
        
        // Set charging LED mode
        power.setChargingLedMode(XPOWERS_CHG_LED_ON);
        
        // Clear any pending PMU interrupts from startup
        power.clearIrqStatus();
    }
    
    // resetActivityTimer(); // Disabled

    // Initialize touch
    touch.setPins(TP_RESET, TP_INT);
    bool touch_ok = touch.begin(Wire, 0x5A, IIC_SDA, IIC_SCL);
    if (touch_ok) {
        USBSerial.println("Touch initialized");
        USBSerial.print("Touch controller model: ");
        USBSerial.println(touch.getModelName());
        touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
        touch.setMirrorXY(true, true);
        
        // Disable auto-sleep to prevent phantom touches
        touch.disableAutoSleep();
        
        // No interrupt needed for simple polling
    } else {
        USBSerial.println("Touch initialization failed");
    }

    // Initialize display
    gfx->begin(30000000); // 30MHz QSPI as per working example
    gfx->fillScreen(BLACK);
    gfx->Display_Brightness(200);
    USBSerial.println("Display initialized");

    // Initialize LVGL
    lv_init();

    // Allocate display buffers - try PSRAM first, then regular memory
    size_t buffer_size = LCD_WIDTH * LCD_HEIGHT / 8;  // 1/8 screen for better memory usage
    
    // Try to allocate in PSRAM first
    buf1 = (lv_color_t *)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t *)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    
    // If PSRAM allocation fails, try regular DMA-capable memory
    if (!buf1 || !buf2) {
        USBSerial.println("PSRAM allocation failed, trying regular memory...");
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        
        buf1 = (lv_color_t *)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
        buf2 = (lv_color_t *)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    }

    if (!buf1 || !buf2) {
        USBSerial.println("Failed to allocate display buffers!");
        USBSerial.print("Buffer size requested: ");
        USBSerial.println(buffer_size * sizeof(lv_color_t));
        while (1);
    }
    
    USBSerial.println("Display buffers allocated successfully");

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_size);

    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = display_flush;
    disp_drv.rounder_cb = display_rounder_cb;  // Add rounder callback
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_drv.long_press_time = 1000;      // 1 second for long press
    indev_drv.long_press_repeat_time = 500; // Repeat every 500ms after long press
    touch_indev = lv_indev_drv_register(&indev_drv);

    // Create timer for LVGL tick
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000);

    // Create the UI
    create_screens();
    
    // Setup GPIO 0 (BOOT button) as secondary wake source
    pinMode(0, INPUT);
    
    // wake_time = millis(); // Disabled

    // Initialize project manager
    USBSerial.println("Initializing project manager...");
    ProjectManager::getInstance().init();
    
    // Synchronize StateMachine with ProjectManager's persisted project selection
    // ProjectManager uses 0-5 indexing: 0="No Project", 1="Work", etc.
    int savedProjectIndex = ProjectManager::getInstance().getSelectedProjectIndex();
    USBSerial.print("Syncing StateMachine with saved project index: ");
    USBSerial.println(savedProjectIndex);
    
    // Initialize state machine
    USBSerial.println("Initializing state machine...");
    stateMachine.begin();
    
    // Set the StateMachine's project index to match ProjectManager
    stateMachine.setSelectedProjectIndex(savedProjectIndex);
    
    // Initialize UI event handler (now mostly unused)
    UIEventHandler::init(&screenManager, &stateMachine);
    
    // Initialize TouchManager for release-only touch processing
    touchManager.init(&stateMachine, &screenManager);
    stateMachine.setTouchManager(&touchManager);
    USBSerial.println("TouchManager initialized - zero touch bleeding guaranteed");
    
    // Initialize simple encoder
    simpleEncoder.begin();
    USBSerial.println("Simple encoder initialized");
    
    // Check if controllers are active
    USBSerial.print("Input controller active: ");
    USBSerial.println(stateMachine.isInputControllerActive() ? "YES" : "NO");
    USBSerial.print("LED controller active: ");
    USBSerial.println(stateMachine.isLEDControllerActive() ? "YES" : "NO");

    // Initialize battery and WiFi icons
    screenManager.updateBatteryIcon();
    USBSerial.println("Battery icon initialized");
    screenManager.updateWifiIcon();
    USBSerial.println("WiFi icon initialized");

    // Register WiFi event handler
    WiFi.onEvent(onWiFiEvent);
    
    // Try to connect to saved WiFi if available
    Preferences wifiPrefs;
    wifiPrefs.begin("wifi", true);
    bool wifiConfigured = wifiPrefs.getBool("configured", false);
    if (wifiConfigured) {
        String ssid = wifiPrefs.getString("ssid", "");
        String password = wifiPrefs.getString("password", "");
        if (ssid.length() > 0) {
            USBSerial.print("Connecting to saved WiFi: ");
            USBSerial.println(ssid);
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid.c_str(), password.c_str());
        }
    }
    wifiPrefs.end();

    USBSerial.println("Setup complete");
    USBSerial.println("--- Integration Testing Available ---");
    USBSerial.println("Commands: 'test' (full tests), 'debug' (encoder), 'led' (LED tests), 'white' (simple LED test)");
    USBSerial.println("         'clearwifi' (clear WiFi credentials)");
}

void loop() {
    // Check for test mode command
    if (USBSerial.available()) {
        String command = USBSerial.readString();
        command.trim();
        USBSerial.print("Received command: '");
        USBSerial.print(command);
        USBSerial.println("'");
        if (command.equalsIgnoreCase("TEST") || command.equalsIgnoreCase("test")) {
            USBSerial.println("=== TEST MODE ACTIVATED ===");
            stateMachine.runTestMode();
            USBSerial.println("=== TEST MODE COMPLETED ===");
            USBSerial.println("Type 'test' again to run tests, or continue normal operation...");
        }
        else if (command.equalsIgnoreCase("DEBUG") || command.equalsIgnoreCase("debug")) {
            USBSerial.println("=== ENCODER DEBUG MODE ===");
            InputController* input = stateMachine.getInputController();
            if (input) {
                input->enableDebugOutput(true);
                input->dumpEncoderState();
                // Register a simple debug handler
                input->onEncoderRotateHandler([](int delta) {
                    USBSerial.print("ENCODER ROTATION: delta=");
                    USBSerial.println(delta);
                });
                USBSerial.println("Debug enabled - rotate encoder to see output");
                USBSerial.println("Send 'test' to run tests or continue normal operation");
            } else {
                USBSerial.println("InputController not available");
            }
        }
        else if (command.equalsIgnoreCase("LED") || command.equalsIgnoreCase("led")) {
            USBSerial.println("=== LED TEST MODE ===");
            LEDController* ledController = stateMachine.getLEDController();
            if (ledController) {
                USBSerial.println("Running LED diagnostic sequence...");
                ledController->runDiagnosticSequence();
                USBSerial.println("LED diagnostic sequence completed");
            } else {
                USBSerial.println("LED controller not available");
            }
        }
        else if (command.equalsIgnoreCase("CLEARWIFI") || command.equalsIgnoreCase("clearwifi")) {
            USBSerial.println("=== CLEARING WIFI CREDENTIALS ===");
            Preferences preferences;
            preferences.begin("wifi", false);
            preferences.clear();
            preferences.end();
            USBSerial.println("WiFi credentials cleared!");
            USBSerial.println("Restarting device...");
            delay(1000);
            ESP.restart();
        }
        else if (command.equalsIgnoreCase("WHITE") || command.equalsIgnoreCase("white")) {
            USBSerial.println("=== SIMPLE WHITE TEST ===");
            LEDController* ledController = stateMachine.getLEDController();
            if (ledController) {
                USBSerial.println("Setting all LEDs to bright white for 3.3V test...");
                ledController->setBrightness(255); // Maximum brightness
                ledController->setSolid(0xFFFFFF); // Pure white
                USBSerial.println("White test active - LEDs should stay bright white");
                USBSerial.println("Testing for 10 seconds...");
                // Use smaller delays with watchdog resets to prevent timeout
                for (int i = 0; i < 10; i++) {
                    delay(1000);
                    esp_task_wdt_reset();
                }
                ledController->turnOff();
                USBSerial.println("White test completed");
            } else {
                USBSerial.println("LED controller not available");
            }
        }
    }
    
    // Skip the entire loop if sleeping
    if (is_sleeping) {
        return;
    }
    
    // Update state machine
    stateMachine.update();
    
    // Handle encoder input for confirmation dialog
    if (screenManager.isConfirmDialogVisible()) {
        int delta = simpleEncoder.readDelta();
        if (delta != 0) {
            bool currentSelection = screenManager.getConfirmSelection();
            screenManager.updateConfirmSelection(!currentSelection); // Toggle selection
            USBSerial.print("Confirmation dialog encoder: switched to ");
            USBSerial.println(screenManager.getConfirmSelection() ? "YES" : "NO");
        }
    }
    
    // Sync LVGL display with state machine state
    static const char* lastStateName = "";
    State* currentState = stateMachine.getCurrentState();
    if (currentState) {
        const char* stateName = currentState->getStateName();
        if (strcmp(stateName, lastStateName) != 0) {
            // State changed - update display
            USBSerial.print("State changed to: ");
            USBSerial.println(stateName);
            
            if (strcmp(stateName, "IdleState") == 0) {
                screenManager.showIdleScreen();
                // Update the idle screen with the current duration (duration is in seconds)
                int durationSeconds = stateMachine.getPendingDuration();
                // Display as MM:SS format for timer
                int minutes = durationSeconds / 60;
                int seconds = durationSeconds % 60;
                screenManager.updateIdleTime(minutes, seconds);
                USBSerial.print("Switched to idle screen with duration: ");
                USBSerial.print(durationSeconds);
                USBSerial.println(" seconds");
            } else if (strcmp(stateName, "AdjustState") == 0) {
                int duration = stateMachine.getPendingDuration();
                screenManager.showAdjustScreen(duration);
                USBSerial.println("Switched to adjust screen");
            } else if (strcmp(stateName, "ProjectSelectState") == 0) {
                screenManager.showProjectScreen();
                USBSerial.println("Switched to project screen");
            } else if (strcmp(stateName, "TimerState") == 0) {
                // TODO: Get actual remaining time and progress
                screenManager.showTimerScreen(0, 0.0f);
                // Project name and color are now set inside showTimerScreen
                USBSerial.println("Switched to timer screen");
            } else if (strcmp(stateName, "PausedState") == 0) {
                // Get remaining time and progress from TimerState if available
                TimerState* timerState = static_cast<TimerState*>(stateMachine.timerState);
                int remainingSeconds = 0;
                float progress = 0.0f;
                if (timerState) {
                    remainingSeconds = timerState->getRemainingSeconds();
                    progress = timerState->getProgressPercentage() / 100.0f;
                    USBSerial.print("Paused with remaining: ");
                    USBSerial.print(remainingSeconds);
                    USBSerial.print(" seconds, progress: ");
                    USBSerial.print(progress * 100);
                    USBSerial.println("%");
                }
                screenManager.showPausedScreen(remainingSeconds, progress);
                USBSerial.println("Switched to paused screen");
            } else if (strcmp(stateName, "DoneState") == 0) {
                screenManager.showDoneScreen();
                USBSerial.println("Switched to done screen");
            }
            lastStateName = stateName;
        }
        
        // Update screen content for states that need continuous updates
        if (strcmp(stateName, "AdjustState") == 0) {
            static int lastAdjustDuration = -1;
            int currentDuration = stateMachine.getPendingDuration();
            if (currentDuration != lastAdjustDuration) {
                screenManager.updateAdjustDuration(currentDuration);
                lastAdjustDuration = currentDuration;
            }
        } else if (strcmp(stateName, "TimerState") == 0) {
            // Cast to TimerState to access timer-specific methods
            TimerState* timerState = static_cast<TimerState*>(currentState);
            if (timerState) {
                unsigned long remaining = timerState->getRemainingSeconds();
                float progress = timerState->getProgressPercentage() / 100.0f;
                screenManager.updateTimerProgress(remaining, progress);
            }
        }
    }
    
    static unsigned long last_print = 0;
    static unsigned long last_transition = 0;
    static bool debug_printed = false;
    unsigned long now = millis();
    
    // Sleep tracking disabled - was causing stack overflow
    
    
    // Check for PMU interrupt by polling the PMU directly
    // This is less efficient but avoids I2C driver conflicts
    static unsigned long last_pmu_check = 0;
    if (now - last_pmu_check > 100) { // Check every 100ms
        last_pmu_check = now;
        // Check if there are any pending interrupts
        if (power.isVbusIn()) {
            // Power connected state changed
        }
    }
    
    // Only check power button when NOT sleeping
    if (!is_sleeping) {
        // Get current interrupt status
        uint32_t irq_status = power.getIrqStatus();
        
        if (irq_status != 0) {
            USBSerial.print("PMU IRQ detected: 0x");
            USBSerial.println(irq_status, HEX);
            
            // Handle power button short press
            if (power.isPekeyShortPressIrq()) {
                USBSerial.println("Power button pressed - going to DEEP sleep");
                power.clearIrqStatus();
                goToSleep(true); // true = deep sleep
            }
            
            // Handle battery status changes for instant battery icon updates
            if (power.isVbusInsertIrq() || power.isVbusRemoveIrq() || 
                power.isBatChargeDoneIrq() || power.isBatChargeStartIrq()) {
                USBSerial.println("Battery status changed - updating icon immediately");
                screenManager.updateBatteryIcon();
            }
            
            // Clear any other interrupts
            power.clearIrqStatus();
        }
    }
    
    // Sleep functionality disabled
    // SLEEP DISABLED - Too annoying during development
    // if (!is_sleeping && time_awake > MIN_AWAKE_TIME && time_since_activity > INACTIVITY_TIMEOUT) {
    //     USBSerial.print("Inactivity timeout - going to light sleep. time_since_activity=");
    //     USBSerial.println(time_since_activity);
    //     goToSleep(false); // Light sleep
    // }
    
    // Print heartbeat every 2 seconds
    if (now - last_print > 2000) {
        State* currentState = stateMachine.getCurrentState();
        const char* stateName = currentState ? currentState->getStateName() : "NULL";
        
        USBSerial.print("Running... State: ");
        USBSerial.print(stateName);
        USBSerial.print(" | Touch: ");
        USBSerial.print(digitalRead(TP_INT) ? "H" : "L");
        USBSerial.println("");
        last_print = now;
    }
    
    // Update battery icon every 30 seconds
    static unsigned long last_battery_update = 0;
    if (now - last_battery_update > 30000) {
        screenManager.updateBatteryIcon();
        last_battery_update = now;
    }
    
    // Update WiFi icon every 10 seconds (more frequent as WiFi status changes more often)
    static unsigned long last_wifi_update = 0;
    if (now - last_wifi_update > 10000) {
        screenManager.updateWifiIcon();
        last_wifi_update = now;
    }
    
    // Auto-transition disabled - use tap to navigate
    // Uncomment below to test automatic transitions
    /*
    if (now - last_transition > 3000 && !transitioning) {
        // Test fade transition
        current_screen = (current_screen + 1) % 3;
        lv_obj_t *target = (current_screen == 0) ? screen1 : (current_screen == 1) ? screen2 : screen3;
        transition_to_screen(target, 600);
        
        last_transition = now;
    }
    */
    
    lv_timer_handler();
    
    // Update breathing effect for paused screen
    screenManager.updatePausedBreathing();
    
    // Handle web server clients
    if (webServerRunning) {
        apiServer.handleClient();
        // webSocket.loop();
    }
    
    delay(5);  // 5ms as per working example
}

// Web Server Implementation
void startWebServer() {
    if (webServerRunning) return;
    
    USBSerial.println("Starting Web Server...");
    
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        USBSerial.println("LittleFS mount failed! Using API-only mode.");
    } else {
        USBSerial.println("LittleFS mounted successfully");
    }
    
    // Start mDNS
    if (MDNS.begin("thetimer")) {
        USBSerial.println("mDNS responder started: http://thetimer.local");
    }
    
    // Configure routes
    apiServer.on("/", HTTP_GET, []() {
        if (LittleFS.exists("/index.html")) {
            fs::File file = LittleFS.open("/index.html", "r");
            apiServer.streamFile(file, "text/html");
            file.close();
        } else {
            apiServer.send(404, "text/plain", "LittleFS not mounted or index.html not found");
        }
    });
    apiServer.on("/style.css", HTTP_GET, []() {
        if (LittleFS.exists("/style.css")) {
            fs::File file = LittleFS.open("/style.css", "r");
            apiServer.streamFile(file, "text/css");
            file.close();
        } else {
            apiServer.send(404, "text/plain", "style.css not found");
        }
    });
    apiServer.on("/app.js", HTTP_GET, []() {
        if (LittleFS.exists("/app.js")) {
            fs::File file = LittleFS.open("/app.js", "r");
            apiServer.streamFile(file, "application/javascript");
            file.close();
        } else {
            apiServer.send(404, "text/plain", "app.js not found");
        }
    });
    apiServer.on("/api/projects", HTTP_GET, handleApiProjects);
    apiServer.on("/api/projects", HTTP_POST, handleApiProjectsPost);
    apiServer.on("/api/updateProject", HTTP_POST, handleApiUpdateProject);
    apiServer.on("/api/deleteProject", HTTP_POST, handleApiDeleteProject);
    apiServer.on("/api/webhook", HTTP_GET, handleApiWebhookGet);
    apiServer.on("/api/webhook", HTTP_POST, handleApiWebhookPost);
    apiServer.on("/api/apikey", HTTP_GET, handleApiKeyGet);
    apiServer.on("/api/apikey", HTTP_POST, handleApiKeyPost);
    apiServer.on("/api/status", HTTP_GET, handleApiStatus);
    apiServer.onNotFound(handleNotFound);
    
    // Start server
    apiServer.begin();
    
    // Start WebSocket server
    // webSocket.begin();
    // webSocket.onEvent(webSocketEvent);
    
    webServerRunning = true;
    
    USBSerial.print("Web Server started at http://");
    USBSerial.print(WiFi.localIP());
    USBSerial.println(" and http://thetimer.local");
    USBSerial.println("WebSocket server started on port 81");
}

void handleApiProjects() {
    StaticJsonDocument<1024> doc;
    JsonArray array = doc.createNestedArray("projects");
    
    ProjectManager& pm = ProjectManager::getInstance();
    for (int i = 0; i < pm.getProjectCount(); i++) {
        const Project* project = pm.getProject(i);
        if (project) {
            JsonObject obj = array.createNestedObject();
            obj["id"] = i;
            obj["name"] = project->name;
            obj["color"] = project->color; // Already a hex string now
            obj["selected"] = (i == pm.getSelectedProjectIndex());
        }
    }
    
    String response;
    serializeJson(doc, response);
    apiServer.send(200, "application/json", response);
}

void handleApiStatus() {
    StaticJsonDocument<256> doc;
    
    State* currentState = stateMachine.getCurrentState();
    doc["state"] = currentState ? currentState->getStateName() : "Unknown";
    doc["project"] = ProjectManager::getInstance().getSelectedProjectName();
    doc["duration"] = stateMachine.getPendingDuration();
    
    // Add timer info if in timer state
    if (strcmp(currentState->getStateName(), "TimerState") == 0) {
        TimerState* timerState = static_cast<TimerState*>(currentState);
        doc["remaining"] = timerState->getRemainingSeconds();
        doc["progress"] = timerState->getProgressPercentage();
    }
    
    String response;
    serializeJson(doc, response);
    apiServer.send(200, "application/json", response);
}

void handleNotFound() {
    apiServer.send(404, "text/plain", "Not Found");
}

// Handle POST /api/projects - Add new project
void handleApiProjectsPost() {
    if (!apiServer.hasArg("plain")) {
        apiServer.send(400, "application/json", "{\"error\":\"No data received\"}");
        return;
    }
    
    String body = apiServer.arg("plain");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        apiServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    const char* name = doc["name"];
    const char* color = doc["color"];
    
    if (!name || !color) {
        apiServer.send(400, "application/json", "{\"error\":\"Missing name or color\"}");
        return;
    }
    
    ProjectManager& pm = ProjectManager::getInstance();
    if (pm.addProject(name, color)) {
        // Refresh the display if we're on the project screen
        if (screenManager.getCurrentScreen() == screenManager.getProjectScreen()) {
            screenManager.refreshProjectDisplay();
        }
        apiServer.send(201, "application/json", "{\"success\":true}");
    } else {
        apiServer.send(400, "application/json", "{\"error\":\"Failed to add project (max reached)\"}");
    }
}

// Handle POST /api/updateProject - Update existing project
void handleApiUpdateProject() {
    if (!apiServer.hasArg("plain")) {
        apiServer.send(400, "application/json", "{\"error\":\"No data received\"}");
        return;
    }
    
    String body = apiServer.arg("plain");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        apiServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    int index = doc["index"] | -1;
    const char* name = doc["name"];
    const char* color = doc["color"];
    
    if (index < 0 || !name || !color) {
        apiServer.send(400, "application/json", "{\"error\":\"Missing index, name or color\"}");
        return;
    }
    
    ProjectManager& pm = ProjectManager::getInstance();
    if (pm.updateProject(index, name, color)) {
        // Refresh the display if we're on the project screen
        if (screenManager.getCurrentScreen() == screenManager.getProjectScreen()) {
            screenManager.refreshProjectDisplay();
        }
        // Also update LED color if this is the currently selected project
        if (index == pm.getSelectedProjectIndex()) {
            uint32_t newColor = pm.getSelectedProjectColor();
            stateMachine.getLEDController()->setSolid(newColor);
        }
        apiServer.send(200, "application/json", "{\"success\":true}");
    } else {
        apiServer.send(404, "application/json", "{\"error\":\"Invalid project index\"}");
    }
}

// Handle POST /api/deleteProject - Delete project
void handleApiDeleteProject() {
    if (!apiServer.hasArg("index")) {
        apiServer.send(400, "application/json", "{\"error\":\"Missing index parameter\"}");
        return;
    }
    
    int index = apiServer.arg("index").toInt();
    
    ProjectManager& pm = ProjectManager::getInstance();
    if (pm.deleteProject(index)) {
        // Refresh the display if we're on the project screen
        if (screenManager.getCurrentScreen() == screenManager.getProjectScreen()) {
            // If we're in project select state, we need to update the bounds
            State* currentState = stateMachine.getCurrentState();
            if (currentState && strcmp(currentState->getStateName(), "ProjectSelectState") == 0) {
                // The project count has changed, need to re-enter the state to reload
                stateMachine.changeState(currentState);
            } else {
                screenManager.refreshProjectDisplay();
            }
        }
        apiServer.send(200, "application/json", "{\"success\":true}");
    } else {
        apiServer.send(404, "application/json", "{\"error\":\"Invalid project index\"}");
    }
}

// Handle GET /api/webhook - Get webhook URL
void handleApiWebhookGet() {
    ProjectManager& pm = ProjectManager::getInstance();
    StaticJsonDocument<256> doc;
    doc["url"] = pm.getWebhookURL();
    
    String response;
    serializeJson(doc, response);
    apiServer.send(200, "application/json", response);
}

// Handle POST /api/webhook - Set webhook URL
void handleApiWebhookPost() {
    if (!apiServer.hasArg("plain")) {
        apiServer.send(400, "application/json", "{\"error\":\"No data received\"}");
        return;
    }
    
    String body = apiServer.arg("plain");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        apiServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    const char* url = doc["url"];
    if (url == nullptr) {
        apiServer.send(400, "application/json", "{\"error\":\"Missing url field\"}");
        return;
    }
    
    ProjectManager& pm = ProjectManager::getInstance();
    pm.setWebhookURL(url);
    apiServer.send(200, "application/json", "{\"message\":\"Webhook URL updated successfully\"}");
}

// Handle GET /api/apikey - Check if API key exists
void handleApiKeyGet() {
    ProjectManager& pm = ProjectManager::getInstance();
    StaticJsonDocument<128> doc;
    doc["key_present"] = !pm.getAPIKey().isEmpty();
    
    String response;
    serializeJson(doc, response);
    apiServer.send(200, "application/json", response);
}

// Handle POST /api/apikey - Set API key
void handleApiKeyPost() {
    if (!apiServer.hasArg("api_key")) {
        apiServer.send(400, "application/json", "{\"error\":\"Missing api_key parameter\"}");
        return;
    }
    
    String apiKey = apiServer.arg("api_key");
    
    ProjectManager& pm = ProjectManager::getInstance();
    pm.setAPIKey(apiKey);
    apiServer.send(200, "application/json", "{\"message\":\"API Key updated successfully\"}");
}

/*
// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            USBSerial.printf("WebSocket client %u disconnected\n", num);
            break;
            
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                USBSerial.printf("WebSocket client %u connected from %s\n", num, ip.toString().c_str());
                webSocket.sendTXT(num, "Connected to TheTimer");
            }
            break;
            
        case WStype_TEXT:
            {
                String message = String((char*)payload);
                USBSerial.printf("WebSocket received: %s\n", message.c_str());
                
                // Parse command
                int colonIndex = message.indexOf(':');
                if (colonIndex > 0) {
                    String command = message.substring(0, colonIndex);
                    String value = message.substring(colonIndex + 1);
                    
                    if (command == "preview-color") {
                        // Update LED to preview color
                        value.trim();
                        if (value.startsWith("#") && value.length() == 7) {
                            long rgb = strtol(value.c_str() + 1, nullptr, 16);
                            uint8_t r = (rgb >> 16) & 0xFF;
                            uint8_t g = (rgb >> 8) & 0xFF;
                            uint8_t b = rgb & 0xFF;
                            
                            // Update LED color temporarily
                            stateMachine.getLEDController()->setColor(r, g, b);
                            webSocket.sendTXT(num, "Color preview updated");
                        }
                    } else if (command == "reset-color") {
                        // Reset LED to current project color
                        ProjectManager& pm = ProjectManager::getInstance();
                        uint32_t color = pm.getSelectedProjectColor();
                        uint8_t r = (color >> 16) & 0xFF;
                        uint8_t g = (color >> 8) & 0xFF;
                        uint8_t b = color & 0xFF;
                        
                        stateMachine.getLEDController()->setColor(r, g, b);
                        webSocket.sendTXT(num, "Color reset to project color");
                    }
                }
            }
            break;
            
        case WStype_BIN:
            USBSerial.printf("WebSocket binary data received (length: %u)\n", length);
            break;
            
        default:
            break;
    }
}
*/

// WiFi Event Handler
void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            USBSerial.println("WiFi connected!");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            USBSerial.print("WiFi got IP: ");
            USBSerial.println(WiFi.localIP());
            startWebServer();
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            USBSerial.println("WiFi disconnected");
            if (webServerRunning) {
                apiServer.stop();
                // webSocket.close();
                webServerRunning = false;
                USBSerial.println("Web Server and WebSocket stopped");
            }
            break;
    }
}

