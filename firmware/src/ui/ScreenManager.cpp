#include "ScreenManager.h"
#include "UIEventHandler.h"
#include "../ProjectManager.h"
#include <Arduino.h>
#include "HWCDC.h"
#include <lvgl.h>
#include "XPowersLib.h"
#include <WiFi.h>
#include "Arduino_GFX_Library.h"

// USB Serial for ESP32-S3
extern HWCDC USBSerial;

// Power management from main file
extern XPowersAXP2101 power;

// Track screen transition timing
unsigned long lastScreenTransitionTime = 0;

ScreenManager::ScreenManager() : 
    idle_screen(nullptr),
    adjust_screen(nullptr),
    project_screen(nullptr),
    timer_screen(nullptr),
    paused_screen(nullptr),
    done_screen(nullptr),
    provision_screen(nullptr),
    current_screen(nullptr),
    idle_time_label(nullptr),
    idle_date_label(nullptr),
    battery_label(nullptr),
    wifi_label(nullptr),
    adjust_duration_label(nullptr),
    adjust_progress_arc(nullptr),
    timer_time_label(nullptr),
    timer_progress_arc(nullptr),
    timer_project_label(nullptr),
    paused_time_label(nullptr),
    paused_progress_arc(nullptr),
    paused_project_label(nullptr),
    done_time_label(nullptr),
    done_progress_arc(nullptr),
    done_complete_label(nullptr),
    project_progress_arc(nullptr),
    confirm_dialog(nullptr),
    confirm_message(nullptr),
    confirm_yes_btn(nullptr),
    confirm_no_btn(nullptr),
    confirm_yes_selected(false), // Default to NO for safety
    currentProjectIndex(0) {
    // Initialize project labels array
    for (int i = 0; i < 5; i++) {
        project_labels[i] = nullptr;
    }
}

ScreenManager::~ScreenManager() {
}

void ScreenManager::init() {
    // Create all screens
    createIdleScreen();
    createAdjustScreen();
    createProjectScreen();
    createTimerScreen();
    createPausedScreen();
    createDoneScreen();
    createProvisionScreen();
    
    // Show idle screen by default
    showIdleScreen();
}

void ScreenManager::createIdleScreen() {
    idle_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(idle_screen, lv_color_black(), 0);
    // No LVGL event handlers - TouchManager handles all touch events
    
    // WiFi icon - positioned for circular display  
    wifi_label = lv_label_create(idle_screen);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_label, COLOR_BACKGROUND, 0);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_24, 0);
    lv_obj_align(wifi_label, LV_ALIGN_TOP_MID, -30, 80);  // More spacing between icons
    
    // Battery icon - positioned for circular display
    battery_label = lv_label_create(idle_screen);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(battery_label, COLOR_BACKGROUND, 0);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_24, 0);
    lv_obj_align(battery_label, LV_ALIGN_TOP_MID, 30, 80);  // More spacing between icons
    
    // Center timer display - truly centered vertically
    lv_obj_t* timer_container = lv_obj_create(idle_screen);
    lv_obj_set_size(timer_container, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_align(timer_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(timer_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(timer_container, 0, 0);
    lv_obj_clear_flag(timer_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(timer_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(timer_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(timer_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Time label (25:00)
    idle_time_label = lv_label_create(timer_container);
    lv_label_set_text(idle_time_label, "25:00");
    lv_obj_set_style_text_color(idle_time_label, COLOR_FOREGROUND, 0);
    lv_obj_set_style_text_font(idle_time_label, &lv_font_roboto_mono_120, 0);
    lv_obj_set_style_text_letter_space(idle_time_label, -12, 0);
    
    // Bottom date label
    lv_obj_t* bottom_row = lv_obj_create(idle_screen);
    lv_obj_set_size(bottom_row, lv_pct(100), lv_pct(33));
    lv_obj_align(bottom_row, LV_ALIGN_BOTTOM_MID, 0, -23);  // Moved up ~5% (466 * 0.05 = ~23 pixels)
    lv_obj_set_style_bg_opa(bottom_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottom_row, 0, 0);
    lv_obj_clear_flag(bottom_row, LV_OBJ_FLAG_CLICKABLE);
    
    idle_date_label = lv_label_create(bottom_row);
    lv_label_set_text(idle_date_label, "Friday, December 15");
    lv_obj_set_style_text_color(idle_date_label, COLOR_BACKGROUND, 0);
    lv_obj_set_style_text_font(idle_date_label, &lv_font_barlow_bold_24, 0);
    lv_obj_center(idle_date_label);
}

void ScreenManager::createAdjustScreen() {
    adjust_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(adjust_screen, lv_color_black(), 0);
    // No LVGL event handlers - TouchManager handles all touch events
    
    // Progress arc - full screen size for circular display
    adjust_progress_arc = lv_arc_create(adjust_screen);
    lv_obj_set_size(adjust_progress_arc, 466, 466); // Full screen size
    lv_obj_align(adjust_progress_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(adjust_progress_arc, 270);
    lv_arc_set_bg_angles(adjust_progress_arc, 0, 360);
    lv_arc_set_angles(adjust_progress_arc, 0, 0); // Will be updated based on duration
    lv_obj_set_style_arc_color(adjust_progress_arc, lv_color_hex(0xDDDDDD), LV_PART_INDICATOR); // Light gray for No Project
    lv_obj_set_style_arc_color(adjust_progress_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN); // Very dark gray
    lv_obj_set_style_arc_width(adjust_progress_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(adjust_progress_arc, 12, LV_PART_MAIN);
    lv_obj_remove_style(adjust_progress_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(adjust_progress_arc, LV_OBJ_FLAG_CLICKABLE);
    
    // Duration display container
    lv_obj_t* duration_container = lv_obj_create(adjust_screen);
    lv_obj_set_size(duration_container, lv_pct(100), lv_pct(34));
    lv_obj_align(duration_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(duration_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(duration_container, 0, 0);
    lv_obj_clear_flag(duration_container, LV_OBJ_FLAG_CLICKABLE);
    
    // Duration label
    adjust_duration_label = lv_label_create(duration_container);
    lv_label_set_text(adjust_duration_label, "25:00");
    lv_obj_set_style_text_color(adjust_duration_label, COLOR_FOREGROUND, 0);
    lv_obj_set_style_text_font(adjust_duration_label, &lv_font_roboto_mono_120, 0);
    lv_obj_set_style_text_letter_space(adjust_duration_label, -12, 0);
    lv_obj_center(adjust_duration_label);
    
}

// Projects are now loaded from ProjectManager dynamically

void ScreenManager::createProjectScreen() {
    project_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(project_screen, lv_color_black(), 0);
    // No LVGL event handlers - TouchManager handles all touch events
    
    // Project color arc - full circle
    project_progress_arc = lv_arc_create(project_screen);
    lv_obj_set_size(project_progress_arc, 466, 466); // Full screen size
    lv_obj_align(project_progress_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(project_progress_arc, 270);
    lv_arc_set_bg_angles(project_progress_arc, 0, 360);
    lv_arc_set_angles(project_progress_arc, 0, 360); // Full circle
    lv_obj_set_style_arc_color(project_progress_arc, lv_color_hex(0x888888), LV_PART_INDICATOR); // Default gray
    lv_obj_set_style_arc_color(project_progress_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN); // Very dark gray
    lv_obj_set_style_arc_width(project_progress_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(project_progress_arc, 12, LV_PART_MAIN);
    lv_obj_remove_style(project_progress_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(project_progress_arc, LV_OBJ_FLAG_CLICKABLE);
    
    // Create a non-clickable container for all labels
    lv_obj_t* project_container = lv_obj_create(project_screen);
    lv_obj_set_size(project_container, lv_pct(100), lv_pct(100));
    lv_obj_align(project_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(project_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(project_container, 0, 0);
    lv_obj_clear_flag(project_container, LV_OBJ_FLAG_CLICKABLE);
    
    // Create 5 labels inside the container
    // Label spacing for circular display
    int y_spacing = 60; // Vertical spacing between labels
    int center_y = 0;   // Center of screen
    
    // Create labels from top to bottom
    for (int i = 0; i < 5; i++) {
        project_labels[i] = lv_label_create(project_container);
        lv_obj_clear_flag(project_labels[i], LV_OBJ_FLAG_CLICKABLE);
        
        // Position label
        int y_offset = (i - 2) * y_spacing; // -2, -1, 0, 1, 2
        lv_obj_align(project_labels[i], LV_ALIGN_CENTER, 0, y_offset);
        
        // Style based on position
        if (i == 2) { // Center label (selected)
            lv_obj_set_style_text_font(project_labels[i], &lv_font_barlow_bold_48, 0);
        } else {
            lv_obj_set_style_text_font(project_labels[i], &lv_font_barlow_bold_24, 0);
            lv_obj_set_style_text_opa(project_labels[i], 102, 0); // 40% opacity
        }
    }
    
    // Initialize with first display
    currentProjectIndex = 0;
    updateProjectDisplay(0);
}

void ScreenManager::createTimerScreen() {
    timer_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(timer_screen, lv_color_black(), 0);
    // No LVGL event handlers - TouchManager handles all touch events
    
    
    // Progress arc - full screen size for circular display
    timer_progress_arc = lv_arc_create(timer_screen);
    lv_obj_set_size(timer_progress_arc, 466, 466); // Full screen size
    lv_obj_align(timer_progress_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(timer_progress_arc, 270);
    lv_arc_set_bg_angles(timer_progress_arc, 0, 360);
    lv_arc_set_angles(timer_progress_arc, 0, 0);
    lv_obj_set_style_arc_color(timer_progress_arc, lv_color_hex(0xDDDDDD), LV_PART_INDICATOR); // Light gray for No Project
    lv_obj_set_style_arc_color(timer_progress_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN); // Very dark gray
    lv_obj_set_style_arc_width(timer_progress_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(timer_progress_arc, 12, LV_PART_MAIN);
    lv_obj_remove_style(timer_progress_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(timer_progress_arc, LV_OBJ_FLAG_CLICKABLE);
    
    // Timer display - absolutely centered like idle screen
    timer_time_label = lv_label_create(timer_screen);
    lv_label_set_text(timer_time_label, "24:35");
    lv_obj_set_style_text_color(timer_time_label, COLOR_FOREGROUND, 0);
    lv_obj_set_style_text_font(timer_time_label, &lv_font_roboto_mono_120, 0);
    lv_obj_set_style_text_letter_space(timer_time_label, -12, 0);
    lv_obj_align(timer_time_label, LV_ALIGN_CENTER, 0, 0); // Absolutely centered
    
    // Project label - positioned between center and bottom, visible and properly spaced
    timer_project_label = lv_label_create(timer_screen);
    lv_label_set_text(timer_project_label, "No Project"); // Default, will be updated dynamically
    lv_obj_set_style_text_color(timer_project_label, COLOR_BACKGROUND, 0); // Default, will be updated with project color
    lv_obj_set_style_text_font(timer_project_label, &lv_font_barlow_bold_48, 0); // Larger font
    lv_obj_align(timer_project_label, LV_ALIGN_CENTER, 0, lv_pct(25)); // 25% below center
    
}

void ScreenManager::createPausedScreen() {
    paused_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(paused_screen, lv_color_black(), 0);
    // No LVGL event handlers - TouchManager handles all touch events
    
    // Progress arc - same as timer screen but dimmed
    paused_progress_arc = lv_arc_create(paused_screen);
    lv_obj_set_size(paused_progress_arc, 466, 466); // Full screen size
    lv_obj_align(paused_progress_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(paused_progress_arc, 270);
    lv_arc_set_bg_angles(paused_progress_arc, 0, 360);
    lv_arc_set_angles(paused_progress_arc, 0, 0); // Will be updated with progress
    lv_obj_set_style_arc_color(paused_progress_arc, lv_color_hex(0xDDDDDD), LV_PART_INDICATOR); // Light gray for No Project
    lv_obj_set_style_arc_color(paused_progress_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN); // Very dark gray
    lv_obj_set_style_arc_width(paused_progress_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(paused_progress_arc, 12, LV_PART_MAIN);
    lv_obj_remove_style(paused_progress_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(paused_progress_arc, LV_OBJ_FLAG_CLICKABLE);
    
    // Timer display - absolutely centered like idle and timer screens
    paused_time_label = lv_label_create(paused_screen);
    lv_label_set_text(paused_time_label, "24:35");
    lv_obj_set_style_text_color(paused_time_label, COLOR_FOREGROUND, 0);
    lv_obj_set_style_text_font(paused_time_label, &lv_font_roboto_mono_120, 0);
    lv_obj_set_style_text_letter_space(paused_time_label, -12, 0);
    lv_obj_align(paused_time_label, LV_ALIGN_CENTER, 0, 0); // Absolutely centered
    
    // Project label - positioned between center and bottom, matching timer screen
    paused_project_label = lv_label_create(paused_screen);
    lv_label_set_text(paused_project_label, "No Project"); // Default, will be updated dynamically
    lv_obj_set_style_text_color(paused_project_label, COLOR_BACKGROUND, 0); // Default, will be updated with project color
    lv_obj_set_style_text_font(paused_project_label, &lv_font_barlow_bold_48, 0); // Same large font as timer screen
    lv_obj_align(paused_project_label, LV_ALIGN_CENTER, 0, lv_pct(25)); // 25% below center
    
}

void ScreenManager::createDoneScreen() {
    done_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(done_screen, lv_color_black(), 0);
    // No LVGL event handlers - TouchManager handles all touch events
    
    // Progress arc - full screen size for circular display, showing 100% complete
    done_progress_arc = lv_arc_create(done_screen);
    lv_obj_set_size(done_progress_arc, 466, 466); // Full screen size
    lv_obj_align(done_progress_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(done_progress_arc, 270);
    lv_arc_set_bg_angles(done_progress_arc, 0, 360);
    lv_arc_set_angles(done_progress_arc, 0, 360); // Full circle to show completion
    lv_obj_set_style_arc_color(done_progress_arc, lv_color_hex(0xDDDDDD), LV_PART_INDICATOR); // Will be updated with project color
    lv_obj_set_style_arc_color(done_progress_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN); // Very dark gray
    lv_obj_set_style_arc_width(done_progress_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(done_progress_arc, 12, LV_PART_MAIN);
    lv_obj_remove_style(done_progress_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(done_progress_arc, LV_OBJ_FLAG_CLICKABLE);
    
    // Timer display - absolutely centered like all other screens showing "0:00" (duration completed)
    done_time_label = lv_label_create(done_screen);
    lv_label_set_text(done_time_label, "0:00");
    lv_obj_set_style_text_color(done_time_label, COLOR_FOREGROUND, 0);
    lv_obj_set_style_text_font(done_time_label, &lv_font_roboto_mono_120, 0);
    lv_obj_set_style_text_letter_space(done_time_label, -12, 0);
    lv_obj_align(done_time_label, LV_ALIGN_CENTER, 0, 0); // Absolutely centered
    
    // "Complete" label - positioned between center and bottom, matching timer screen
    done_complete_label = lv_label_create(done_screen);
    lv_label_set_text(done_complete_label, "Complete");
    lv_obj_set_style_text_color(done_complete_label, COLOR_FOREGROUND, 0); // Will be updated with project color
    lv_obj_set_style_text_font(done_complete_label, &lv_font_barlow_bold_48, 0); // Same large font as project label
    lv_obj_align(done_complete_label, LV_ALIGN_CENTER, 0, lv_pct(25)); // 25% below center
}

void ScreenManager::createProvisionScreen() {
    provision_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(provision_screen, lv_color_black(), 0);
    
    // Create WiFi icon
    lv_obj_t* wifi_icon = lv_label_create(provision_screen);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(wifi_icon, COLOR_FOREGROUND, 0);
    lv_obj_align(wifi_icon, LV_ALIGN_CENTER, 0, -100);
    
    // Create instructions
    lv_obj_t* instructions = lv_label_create(provision_screen);
    lv_label_set_text(instructions, "Connect to WiFi:");
    lv_obj_set_style_text_font(instructions, &lv_font_barlow_24, 0);
    lv_obj_set_style_text_color(instructions, COLOR_FOREGROUND, 0);
    lv_obj_align(instructions, LV_ALIGN_CENTER, 0, -10);
    
    // Create AP name label (will be updated dynamically)
    lv_obj_t* ap_label = lv_label_create(provision_screen);
    lv_label_set_text(ap_label, "TheTimer");
    lv_obj_set_style_text_font(ap_label, &lv_font_barlow_bold_24, 0);
    lv_obj_set_style_text_color(ap_label, COLOR_FOREGROUND, 0);
    lv_obj_align(ap_label, LV_ALIGN_CENTER, 0, 20);
    
    // Create URL label
    lv_obj_t* url_label = lv_label_create(provision_screen);
    lv_label_set_text(url_label, "192.168.4.1");
    lv_obj_set_style_text_font(url_label, &lv_font_barlow_24, 0);
    lv_obj_set_style_text_color(url_label, COLOR_BACKGROUND, 0);
    lv_obj_align(url_label, LV_ALIGN_CENTER, 0, 60);
}

void ScreenManager::createPageIndicator(lv_obj_t* parent, int active_index, int total_pages) {
    // Removed - no longer needed
}

void ScreenManager::showIdleScreen(int direction) {
    if (idle_screen) {
        USBSerial.print("[SCREEN] Loading idle screen: 0x");
        USBSerial.println((uint32_t)idle_screen, HEX);
        lv_scr_load_anim(idle_screen, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
        current_screen = idle_screen;
        lastScreenTransitionTime = millis();
        USBSerial.println("[SCREEN] Idle screen loaded");
    }
}

void ScreenManager::showAdjustScreen(int duration) {
    if (adjust_screen) {
        updateAdjustDuration(duration);
        
        // Update arc color based on selected project
        ProjectManager& pm = ProjectManager::getInstance();
        uint32_t projectColor = pm.getSelectedProjectColor();
        
        if (adjust_progress_arc) {
            lv_obj_set_style_arc_color(adjust_progress_arc, lv_color_hex(projectColor), LV_PART_INDICATOR);
        }
        
        lv_scr_load_anim(adjust_screen, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
        current_screen = adjust_screen;
    }
}

void ScreenManager::showProjectScreen() {
    if (project_screen) {
        USBSerial.print("[SCREEN] Loading project screen: 0x");
        USBSerial.println((uint32_t)project_screen, HEX);
        
        // Set the current project index to the saved project
        ProjectManager& pm = ProjectManager::getInstance();
        int savedIndex = pm.getSelectedProjectIndex();
        updateProjectDisplay(savedIndex);
        USBSerial.print("[SCREEN] Project screen starting with saved project: ");
        USBSerial.print(pm.getSelectedProjectName());
        USBSerial.print(" (index: ");
        USBSerial.print(savedIndex);
        USBSerial.println(")");
        
        lv_scr_load_anim(project_screen, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
        current_screen = project_screen;
        lastScreenTransitionTime = millis();
        USBSerial.println("[SCREEN] Project screen loaded");
    }
}

void ScreenManager::showTimerScreen(int remaining_seconds, float progress) {
    if (timer_screen) {
        USBSerial.print("[SCREEN] Loading timer screen: 0x");
        USBSerial.println((uint32_t)timer_screen, HEX);
        updateTimerProgress(remaining_seconds, progress);
        
        // Update arc color based on selected project
        ProjectManager& pm = ProjectManager::getInstance();
        uint32_t projectColor = pm.getSelectedProjectColor();
        const char* projectName = pm.getSelectedProjectName();
        
        // Always ensure arc is fully visible on timer screen
        if (timer_progress_arc) {
            lv_obj_clear_flag(timer_progress_arc, LV_OBJ_FLAG_HIDDEN); // Show arc when unpaused
            lv_obj_set_style_arc_color(timer_progress_arc, lv_color_hex(projectColor), LV_PART_INDICATOR);
            lv_obj_set_style_arc_opa(timer_progress_arc, 255, LV_PART_INDICATOR);
            lv_obj_set_style_arc_opa(timer_progress_arc, 255, LV_PART_MAIN);
        }
        
        // Update project name
        updateTimerProject(projectName);
        
        lv_scr_load_anim(timer_screen, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
        current_screen = timer_screen;
        lastScreenTransitionTime = millis();
        USBSerial.println("[SCREEN] Timer screen loaded");
    }
}

void ScreenManager::showPausedScreen(int remaining_seconds, float progress) {
    if (paused_screen) {
        USBSerial.print("[SCREEN] Loading paused screen: 0x");
        USBSerial.println((uint32_t)paused_screen, HEX);
        // Update time display
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", 
                 remaining_seconds / 60, remaining_seconds % 60);
        lv_label_set_text(paused_time_label, time_str);
        
        // Update project label with pause symbol
        ProjectManager& pm = ProjectManager::getInstance();
        uint32_t projectColor = pm.getSelectedProjectColor();
        lv_label_set_text(paused_project_label, LV_SYMBOL_PAUSE); // Font Awesome pause icon
        lv_obj_set_style_text_font(paused_project_label, &lv_font_montserrat_48, 0); // Use Montserrat for symbols
        lv_obj_set_style_text_color(paused_project_label, lv_color_hex(projectColor), 0);
        
        // Hide arc when paused
        if (paused_progress_arc) {
            lv_obj_add_flag(paused_progress_arc, LV_OBJ_FLAG_HIDDEN);
        }
        
        lv_scr_load_anim(paused_screen, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
        current_screen = paused_screen;
        lastScreenTransitionTime = millis();
        USBSerial.println("[SCREEN] Paused screen loaded");
    }
}

void ScreenManager::showDoneScreen() {
    if (done_screen) {
        // Update arc color to match current project
        if (done_progress_arc) {
            lv_obj_set_style_arc_color(done_progress_arc, lv_color_hex(getCurrentProjectColor()), LV_PART_INDICATOR);
        }
        
        // Update "TIMER COMPLETE" text color to match project
        if (done_complete_label) {
            lv_obj_set_style_text_color(done_complete_label, lv_color_hex(getCurrentProjectColor()), 0);
        }
        
        lv_scr_load_anim(done_screen, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
        current_screen = done_screen;
        lastScreenTransitionTime = millis();
    }
}

void ScreenManager::updateIdleTime(int hours, int minutes) {
    if (idle_time_label) {
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", hours, minutes);
        lv_label_set_text(idle_time_label, time_str);
    }
}

void ScreenManager::updateAdjustDuration(int seconds) {
    if (adjust_duration_label) {
        char time_str[16];
        if (seconds == 0) {
            // Display Font Awesome loop symbol for indeterminate timer
            snprintf(time_str, sizeof(time_str), LV_SYMBOL_LOOP);
        } else if (seconds == 10) {
            // Special case: 10 seconds for testing
            snprintf(time_str, sizeof(time_str), "0:10");
        } else if (seconds >= 3600) {
            // Display as H:MM format for 60+ minutes
            int hours = seconds / 3600;
            int mins = (seconds % 3600) / 60;
            snprintf(time_str, sizeof(time_str), "%d:%02d", hours, mins);
        } else {
            // Display as MM:00 format for timer durations (show minutes, not actual seconds)
            int mins = seconds / 60;
            snprintf(time_str, sizeof(time_str), "%02d:00", mins);
            USBSerial.print("DISPLAY DEBUG: ");
            USBSerial.print(seconds);
            USBSerial.print(" seconds -> ");
            USBSerial.print(mins);
            USBSerial.print(" minutes -> ");
            USBSerial.println(time_str);
        }
        lv_label_set_text(adjust_duration_label, time_str);
        
        // Set appropriate font based on content
        if (seconds == 0) {
            // Use Montserrat for Font Awesome symbols
            lv_obj_set_style_text_font(adjust_duration_label, &lv_font_montserrat_48, 0);
        } else {
            // Use Roboto Mono for numbers
            lv_obj_set_style_text_font(adjust_duration_label, &lv_font_roboto_mono_120, 0);
        }
        
        // Debug
        USBSerial.print("Updated adjust screen to: ");
        USBSerial.println(time_str);
    }
    
    // Update arc progress (0 seconds = 0%, 2 hours = 100%)
    if (adjust_progress_arc) {
        float progress;
        if (seconds == 0) {
            // Indeterminate: show full circle
            progress = 1.0f;
        } else {
            progress = (float)seconds / (120.0f * 60.0f); // 2 hours max in seconds
        }
        if (progress > 1.0f) progress = 1.0f; // Cap at 100%
        int angle = (int)(progress * 360.0f);
        lv_arc_set_angles(adjust_progress_arc, 0, angle);
        
        USBSerial.print("Arc progress: ");
        USBSerial.print(progress * 100);
        USBSerial.print("% (angle: ");
        USBSerial.print(angle);
        USBSerial.println(" degrees)");
    }
}

void ScreenManager::updateTimerProgress(int remaining_seconds, float progress) {
    if (timer_time_label) {
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", 
                 remaining_seconds / 60, remaining_seconds % 60);
        lv_label_set_text(timer_time_label, time_str);
    }
    
    if (timer_progress_arc) {
        // Progress goes from 0 to 360 degrees with minimum visible arc
        int angle = (int)(progress * 360.0f);
        // Ensure minimum 1 degree visible so timer looks active immediately
        if (angle < 1) angle = 1;
        lv_arc_set_angles(timer_progress_arc, 0, angle);
        
        // Maintain project color
        ProjectManager& pm = ProjectManager::getInstance();
        uint32_t projectColor = pm.getSelectedProjectColor();
        lv_obj_set_style_arc_color(timer_progress_arc, lv_color_hex(projectColor), LV_PART_INDICATOR);
    }
}

void ScreenManager::updateTimerProject(const char* projectName) {
    if (timer_project_label) {
        lv_label_set_text(timer_project_label, projectName);
        
        // Update color to match project
        ProjectManager& pm = ProjectManager::getInstance();
        uint32_t projectColor = pm.getSelectedProjectColor();
        lv_obj_set_style_text_color(timer_project_label, lv_color_hex(projectColor), 0);
    }
}

void ScreenManager::updatePausedTime(int remaining_seconds) {
    if (paused_time_label) {
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", 
                 remaining_seconds / 60, remaining_seconds % 60);
        lv_label_set_text(paused_time_label, time_str);
    }
}

void ScreenManager::updatePausedBreathing() {
    // Only breathe text on paused screen - no arc animations
    if (current_screen != paused_screen) return;
    
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastUpdate < 33) return; // ~30fps is fine for breathing
    lastUpdate = currentTime;
    
    // Simple breathing effect for text
    float time_factor = (currentTime % 2000) / 2000.0f;
    float sine_wave = (sin(time_factor * 2 * PI) + 1.0f) / 2.0f;
    int opacity = 180 + (int)(sine_wave * 75); // 70% to 100%
    
    if (paused_time_label) {
        lv_obj_set_style_text_opa(paused_time_label, opacity, 0);
    }
    if (paused_project_label) {
        lv_obj_set_style_text_opa(paused_project_label, opacity, 0);
    }
}

void ScreenManager::updateDoneBreathing() {
    // Only breathe text on done screen - no arc animations
    if (current_screen != done_screen) return;
    
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastUpdate < 33) return; // ~30fps is fine for breathing
    lastUpdate = currentTime;
    
    // Simple breathing effect for text with slower cycle for completion
    float time_factor = (currentTime % 3000) / 3000.0f; // 3 second cycle for more relaxed breathing
    float sine_wave = (sin(time_factor * 2 * PI) + 1.0f) / 2.0f;
    int opacity = 150 + (int)(sine_wave * 105); // 60% to 100% opacity for more dramatic effect
    
    if (done_time_label) {
        lv_obj_set_style_text_opa(done_time_label, opacity, 0);
    }
    if (done_complete_label) {
        lv_obj_set_style_text_opa(done_complete_label, opacity, 0);
    }
}

void ScreenManager::updateProjectDisplay(int newIndex) {
    ProjectManager& pm = ProjectManager::getInstance();
    int projectCount = pm.getProjectCount();
    
    // Validate index
    if (newIndex < 0 || newIndex >= projectCount) return;
    
    currentProjectIndex = newIndex;
    
    // Calculate which projects to show
    int indices[5];
    for (int i = 0; i < 5; i++) {
        int offset = i - 2; // -2, -1, 0, 1, 2
        int idx = (currentProjectIndex + offset + projectCount) % projectCount; // Wrap around
        indices[i] = idx;
    }
    
    // Update labels
    for (int i = 0; i < 5; i++) {
        if (project_labels[i]) {
            const Project* project = pm.getProject(indices[i]);
            if (project) {
                lv_label_set_text(project_labels[i], project->name.c_str());
                
                // Update color and opacity based on position
                if (i == 2) { // Center label (selected)
                    uint32_t color = pm.hexToRGB(project->color);
                    lv_obj_set_style_text_color(project_labels[i], lv_color_hex(color), 0);
                    lv_obj_set_style_text_opa(project_labels[i], 255, 0); // Full opacity
                } else {
                    lv_obj_set_style_text_color(project_labels[i], lv_color_hex(0x888888), 0); // Gray
                    lv_obj_set_style_text_opa(project_labels[i], 102, 0); // 40% opacity
                }
            }
        }
    }
    
    // Update arc color to match selected project
    if (project_progress_arc) {
        const Project* selectedProject = pm.getProject(currentProjectIndex);
        if (selectedProject) {
            uint32_t color = pm.hexToRGB(selectedProject->color);
            lv_obj_set_style_arc_color(project_progress_arc, lv_color_hex(color), LV_PART_INDICATOR);
        }
    }
}

const char* ScreenManager::getCurrentProjectName() {
    ProjectManager& pm = ProjectManager::getInstance();
    const Project* project = pm.getProject(currentProjectIndex);
    if (project) {
        return project->name.c_str();
    }
    return "No Project";
}

uint32_t ScreenManager::getCurrentProjectColor() {
    ProjectManager& pm = ProjectManager::getInstance();
    const Project* project = pm.getProject(currentProjectIndex);
    if (project) {
        return pm.hexToRGB(project->color);
    }
    return 0x888888; // Default gray
}

void ScreenManager::showConfirmDialog(const char* message) {
    if (confirm_dialog) {
        lv_obj_del(confirm_dialog);
    }
    
    // Reset to NO selection for safety
    confirm_yes_selected = false;
    
    // Create modal background - full screen coverage
    confirm_dialog = lv_obj_create(lv_scr_act());
    lv_obj_set_size(confirm_dialog, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(confirm_dialog, 0, 0); // Ensure it starts at exact screen edge
    lv_obj_set_style_bg_color(confirm_dialog, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(confirm_dialog, 230, 0); // Less transparent (was 200)
    lv_obj_set_style_pad_all(confirm_dialog, 0, 0); // Remove any padding
    lv_obj_set_style_border_width(confirm_dialog, 0, 0); // Remove any border
    
    // Message label
    confirm_message = lv_label_create(confirm_dialog);
    lv_label_set_text(confirm_message, message);
    lv_obj_set_style_text_color(confirm_message, COLOR_FOREGROUND, 0);
    lv_obj_set_style_text_font(confirm_message, &lv_font_barlow_bold_24, 0);
    lv_obj_align(confirm_message, LV_ALIGN_CENTER, 0, -30);
    
    // YES button
    confirm_yes_btn = lv_label_create(confirm_dialog);
    lv_label_set_text(confirm_yes_btn, "YES");
    lv_obj_set_style_text_font(confirm_yes_btn, &lv_font_barlow_bold_48, 0);
    lv_obj_align(confirm_yes_btn, LV_ALIGN_CENTER, -50, 30);
    
    // NO button
    confirm_no_btn = lv_label_create(confirm_dialog);
    lv_label_set_text(confirm_no_btn, "NO");
    lv_obj_set_style_text_font(confirm_no_btn, &lv_font_barlow_bold_48, 0);
    lv_obj_align(confirm_no_btn, LV_ALIGN_CENTER, 50, 30);
    
    // Update colors based on initial selection (NO)
    updateConfirmSelection(confirm_yes_selected);
}

void ScreenManager::hideConfirmDialog() {
    if (confirm_dialog) {
        lv_obj_del(confirm_dialog);
        confirm_dialog = nullptr;
        confirm_message = nullptr;
        confirm_yes_btn = nullptr;
        confirm_no_btn = nullptr;
    }
}

bool ScreenManager::isConfirmDialogVisible() {
    return confirm_dialog != nullptr;
}

void ScreenManager::updateConfirmSelection(bool yesSelected) {
    confirm_yes_selected = yesSelected;
    
    if (confirm_yes_btn && confirm_no_btn) {
        if (yesSelected) {
            // YES selected: red color, NO grayed out
            lv_obj_set_style_text_color(confirm_yes_btn, lv_color_hex(0xFF0000), 0); // Red
            lv_obj_set_style_text_color(confirm_no_btn, COLOR_BACKGROUND, 0); // Gray
        } else {
            // NO selected: green color, YES grayed out  
            lv_obj_set_style_text_color(confirm_yes_btn, COLOR_BACKGROUND, 0); // Gray
            lv_obj_set_style_text_color(confirm_no_btn, lv_color_hex(0x00FF00), 0); // Green
        }
    }
}

bool ScreenManager::getConfirmSelection() {
    return confirm_yes_selected;
}


void ScreenManager::showProvisionScreen(const String& apName) {
    if (!provision_screen) return;
    
    // Update AP name label (third child - after wifi icon and instructions)
    lv_obj_t* ap_label = lv_obj_get_child(provision_screen, 2);
    if (ap_label && lv_obj_check_type(ap_label, &lv_label_class)) {
        lv_label_set_text(ap_label, apName.c_str());
    }
    
    // Show the provision screen
    lv_scr_load_anim(provision_screen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
    current_screen = provision_screen;
}

void ScreenManager::showProvisionError(const String& message) {
    // Update existing provision screen with error message
    if (!provision_screen) return;
    
    // Find and update the instructions label (second child)
    lv_obj_t* instructions = lv_obj_get_child(provision_screen, 1);
    if (instructions && lv_obj_check_type(instructions, &lv_label_class)) {
        lv_label_set_text(instructions, message.c_str());
        lv_obj_set_style_text_color(instructions, lv_color_hex(0xFF4444), 0); // Red color for error
    }
}

void ScreenManager::updateBatteryIcon() {
    if (!battery_label) {
        return; // Battery label not initialized
    }
    
    // Get battery status
    bool isCharging = power.isVbusIn(); // Check if USB power connected
    float batteryVoltage = power.getBattVoltage(); // Get battery voltage
    
    // Convert voltage to percentage (approximate)
    // Li-ion: 4.2V = 100%, 3.7V = 50%, 3.3V = 0%
    int batteryPercent = 0;
    if (batteryVoltage >= 4.1f) {
        batteryPercent = 100;
    } else if (batteryVoltage >= 3.9f) {
        batteryPercent = 80;
    } else if (batteryVoltage >= 3.8f) {
        batteryPercent = 60;
    } else if (batteryVoltage >= 3.7f) {
        batteryPercent = 40;
    } else if (batteryVoltage >= 3.5f) {
        batteryPercent = 20;
    } else {
        batteryPercent = 0;
    }
    
    // Always show battery level icon (like Apple does)
    const char* batteryIcon;
    if (batteryPercent >= 80) {
        batteryIcon = LV_SYMBOL_BATTERY_FULL;
    } else if (batteryPercent >= 60) {
        batteryIcon = LV_SYMBOL_BATTERY_3;
    } else if (batteryPercent >= 40) {
        batteryIcon = LV_SYMBOL_BATTERY_2;
    } else if (batteryPercent >= 20) {
        batteryIcon = LV_SYMBOL_BATTERY_1;
    } else {
        batteryIcon = LV_SYMBOL_BATTERY_EMPTY;
    }
    
    // Update battery icon (always visible)
    lv_label_set_text(battery_label, batteryIcon);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_24, 0);
    
    // Update color - green when charging, red when low, gray when normal
    if (isCharging) {
        lv_obj_set_style_text_color(battery_label, lv_color_hex(0x00FF00), 0); // Green when charging
    } else if (batteryPercent <= 20) {
        lv_obj_set_style_text_color(battery_label, lv_color_hex(0xFF0000), 0); // Red when low
    } else {
        lv_obj_set_style_text_color(battery_label, COLOR_BACKGROUND, 0); // Normal gray
    }
}

void ScreenManager::updateWifiIcon() {
    if (!wifi_label) {
        return; // WiFi label not initialized
    }
    
    // Get WiFi status
    wl_status_t status = WiFi.status();
    int rssi = WiFi.RSSI(); // Signal strength
    
    // Determine WiFi icon and color based on status
    const char* wifiIcon;
    lv_color_t iconColor;
    
    if (status == WL_CONNECTED) {
        // Connected - show signal strength
        if (rssi >= -50) {
            wifiIcon = LV_SYMBOL_WIFI; // Full signal
            iconColor = lv_color_hex(0x00FF00); // Green for excellent signal
        } else if (rssi >= -70) {
            wifiIcon = LV_SYMBOL_WIFI; // Good signal
            iconColor = COLOR_FOREGROUND; // White for good signal
        } else {
            wifiIcon = LV_SYMBOL_WIFI; // Weak signal
            iconColor = lv_color_hex(0xFFAA00); // Orange for weak signal
        }
    } else if (status == WL_DISCONNECTED || status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED) {
        // Disconnected or connection failed
        wifiIcon = LV_SYMBOL_WIFI;
        iconColor = COLOR_BACKGROUND; // Gray when disconnected
    } else if (status == WL_CONNECTION_LOST) {
        // Lost connection
        wifiIcon = LV_SYMBOL_WIFI;
        iconColor = lv_color_hex(0xFF0000); // Red for lost connection
    } else {
        // Other states (connecting, etc.)
        wifiIcon = LV_SYMBOL_WIFI;
        iconColor = COLOR_BACKGROUND; // Dim white when connecting
    }
    
    // Update WiFi icon
    lv_label_set_text(wifi_label, wifiIcon);
    lv_obj_set_style_text_color(wifi_label, iconColor, 0);
}

// Display power control methods
void ScreenManager::setDisplayPower(bool on) {
    // Access the global gfx object from firmware.ino
    extern void* gfx_ptr; // We'll define this in firmware.ino
    if (gfx_ptr) {
        Arduino_GFX* gfx = static_cast<Arduino_GFX*>(gfx_ptr);
        if (on) {
            USBSerial.println("[SCREEN] Turning display ON");
            gfx->Display_Brightness(200); // Normal brightness
            gfx->displayOn();  // Also call displayOn
        } else {
            USBSerial.println("[SCREEN] Turning display OFF");
            gfx->Display_Brightness(0);   // Turn off backlight
            gfx->displayOff(); // Also call displayOff
        }
    } else {
        USBSerial.println("[SCREEN] ERROR: gfx_ptr is NULL!");
    }
}

void ScreenManager::turnOffDisplay() {
    setDisplayPower(false);
}

void ScreenManager::turnOnDisplay() {
    setDisplayPower(true);
}