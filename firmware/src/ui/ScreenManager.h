#pragma once

#include <lvgl.h>
#include <Arduino.h>

// Color definitions
#define COLOR_FOREGROUND lv_color_hex(0xDDDDDD)
#define COLOR_BACKGROUND lv_color_hex(0x555555)  // Even darker gray
#define COLOR_DARK_BG lv_color_hex(0x333333)

// Font declarations
LV_FONT_DECLARE(lv_font_roboto_mono_120);
LV_FONT_DECLARE(lv_font_barlow_24);
LV_FONT_DECLARE(lv_font_barlow_bold_24);
LV_FONT_DECLARE(lv_font_barlow_bold_48);

class ScreenManager {
public:
    ScreenManager();
    ~ScreenManager();
    
    void init();
    void showIdleScreen(int direction = 0); // 0=fade, -1=from right, 1=from left
    void showAdjustScreen(int duration);
    void showProjectScreen();
    void showTimerScreen(int remaining_seconds, float progress);
    void showPausedScreen(int remaining_seconds, float progress = 0.5f);
    void showDoneScreen();
    void showProvisionScreen(const String& apName);
    void showProvisionError(const String& message);
    
    // Confirmation dialog
    void showConfirmDialog(const char* message);
    void hideConfirmDialog();
    bool isConfirmDialogVisible();
    void updateConfirmSelection(bool yesSelected);
    bool getConfirmSelection(); // true = YES, false = NO
    
    // Update methods for dynamic content
    void updateIdleTime(int hours, int minutes);
    void updateAdjustDuration(int minutes);
    void updateTimerProgress(int remaining_seconds, float progress);
    void updateTimerProject(const char* projectName);
    void updatePausedTime(int remaining_seconds);
    void updatePausedBreathing(); // Update breathing effect on paused screen
    void updateDoneBreathing(); // Update breathing effect on done screen
    void updateBatteryIcon(); // Update battery icon based on current status
    void updateWifiIcon(); // Update WiFi icon based on connection status
    
    lv_obj_t* getCurrentScreen() { return current_screen; }
    lv_obj_t* getIdleScreen() { return idle_screen; }
    lv_obj_t* getProjectScreen() { return project_screen; }
    lv_obj_t* getTimerScreen() { return timer_screen; }
    lv_obj_t* getPausedScreen() { return paused_screen; }
    
    // Project navigation
    void updateProjectDisplay(int newIndex);
    int getCurrentProjectIndex() { return currentProjectIndex; }
    const char* getCurrentProjectName();
    uint32_t getCurrentProjectColor();
    
    // Refresh project display with current data
    void refreshProjectDisplay() { updateProjectDisplay(currentProjectIndex); }
    
private:
    // Project selection
    int currentProjectIndex;
    lv_obj_t* project_labels[5]; // prev2, prev1, center, next1, next2
    lv_obj_t* project_progress_arc;
    
    // Screens
    lv_obj_t* idle_screen;
    lv_obj_t* adjust_screen;
    lv_obj_t* project_screen;
    lv_obj_t* timer_screen;
    lv_obj_t* paused_screen;
    lv_obj_t* done_screen;
    lv_obj_t* provision_screen;
    
    // Current active screen
    lv_obj_t* current_screen;
    
    // Dynamic content labels
    lv_obj_t* idle_time_label;
    lv_obj_t* idle_date_label;
    lv_obj_t* battery_label; // Battery icon for dynamic updates
    lv_obj_t* wifi_label; // WiFi icon for dynamic updates
    lv_obj_t* adjust_duration_label;
    lv_obj_t* adjust_progress_arc;
    lv_obj_t* timer_time_label;
    lv_obj_t* timer_progress_arc;
    lv_obj_t* timer_project_label;
    lv_obj_t* paused_time_label;
    lv_obj_t* paused_progress_arc;
    lv_obj_t* paused_project_label;
    lv_obj_t* done_time_label;
    lv_obj_t* done_progress_arc;
    lv_obj_t* done_complete_label;
    
    // Confirmation dialog elements
    lv_obj_t* confirm_dialog;
    lv_obj_t* confirm_message;
    lv_obj_t* confirm_yes_btn;
    lv_obj_t* confirm_no_btn;
    bool confirm_yes_selected;
    
    // Helper methods
    void createIdleScreen();
    void createAdjustScreen();
    void createProjectScreen();
    void createTimerScreen();
    void createPausedScreen();
    void createDoneScreen();
    void createProvisionScreen();
    
    void createPageIndicator(lv_obj_t* parent, int active_index, int total_pages);
};