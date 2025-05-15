#include "StateMachine.h"
#include "Controllers.h"
#include "managers/ProjectManager.h"
#include "states/DoneState.h"
#include <lvgl.h>

// Static LVGL event handler for DoneState
static void done_screen_tap_event_handler(lv_event_t *e);

DoneState::DoneState() : 
    finalElapsedTimeSeconds(0),
    completedProjectId(""),
    completedProjectName("No Project"),
    completedProjectColor(LEDController::hexColorToUint32("#FFFFFF")),
    doneLabel(nullptr),
    timeDisplayLabel(nullptr),
    projectNameLabel(nullptr),
    instructionLabel(nullptr) {}

void DoneState::processScreenTap() {
    Serial.println("DoneState: Screen Tapped, returning to IdleState.");
    stateMachine.clearPendingProject(); // Clear project ID from state machine context
    stateMachine.changeState(&StateMachine::idleState);
}

static void done_screen_tap_event_handler(lv_event_t *e) {
    DoneState* self = (DoneState*)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && self) {
        if (millis() - self->getEntryTime() < State::TAP_DEBOUNCE_MS) {
            Serial.println("DoneState: Tap ignored (debounce)");
            return;
        }
        self->processScreenTap();
    }
}

void DoneState::enter() {
    State::enter();
    Serial.println("Entering Done State");

    finalElapsedTimeSeconds = stateMachine.getPendingElapsedTime();
    completedProjectId = stateMachine.getPendingProjectId();
    completedProjectName = "No Project";
    String projectColorHex = "#FFFFFF"; // Default white

    if (!completedProjectId.isEmpty()) {
        bool found = false;
        const auto &allProjects = getProjectManagerInstance().getProjects();
        for (const auto &p : allProjects) {
            if (p.device_project_id == completedProjectId) {
                completedProjectName = p.name;
                projectColorHex = p.color;
                found = true;
                break;
            }
        }
        if (!found) {
            Serial.printf("DoneState: Project ID '%s' not found, using defaults.\n", completedProjectId.c_str());
            completedProjectName = "Task Finished"; // Generic if ID was set but not found
        } else {
             Serial.printf("DoneState: Project '%s' (ID: %s) finished.\n", completedProjectName.c_str(), completedProjectId.c_str());
        }
    } else {
        Serial.println("DoneState: No project ID, general completion.");
        completedProjectName = "Task Finished";
    }
    completedProjectColor = LEDController::hexColorToUint32(projectColorHex);

    // LED indication: Solid color of the project, or a celebratory flash then solid
    ledController.setSolid(completedProjectColor); // Simple solid color for now
    // Or: ledController.flash(completedProjectColor, 3, 200); // Flash 3 times

    lv_obj_t *screen = lv_screen_active();
    if (!screen) { Serial.println("DoneState::enter() - FATAL: screen is NULL"); return; }
    lv_obj_clean(screen);

    doneLabel = lv_label_create(screen);
    lv_label_set_text(doneLabel, "Timer Complete!");
    lv_obj_set_style_text_font(doneLabel, &lv_font_montserrat_14, 0); // Consider a larger font
    lv_obj_align(doneLabel, LV_ALIGN_TOP_MID, 0, 10);

    projectNameLabel = lv_label_create(screen);
    lv_label_set_text(projectNameLabel, completedProjectName.c_str());
    lv_obj_set_style_text_font(projectNameLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(projectNameLabel, LV_ALIGN_TOP_MID, 0, 35);

    timeDisplayLabel = lv_label_create(screen);
    char timeStr[20]; // Increased size for "Total: ..."
    int hours = finalElapsedTimeSeconds / 3600;
    int minutes = (finalElapsedTimeSeconds % 3600) / 60;
    int seconds = finalElapsedTimeSeconds % 60;
    if (hours > 0) {
        sprintf(timeStr, "Total: %dh %02dm", hours, minutes);
    } else if (minutes > 0) {
        sprintf(timeStr, "Total: %dm %02ds", minutes, seconds);
    } else {
        sprintf(timeStr, "Total: %ds", seconds);
    }
    lv_label_set_text(timeDisplayLabel, timeStr);
    lv_obj_set_style_text_font(timeDisplayLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(timeDisplayLabel, LV_ALIGN_CENTER, 0, 0);

    instructionLabel = lv_label_create(screen);
    lv_label_set_text(instructionLabel, "Tap to Continue");
    lv_obj_set_style_text_font(instructionLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(instructionLabel, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, done_screen_tap_event_handler, LV_EVENT_CLICKED, this);
    
    Serial.println("DoneState::enter - UI created, event handler added.");
}

void DoneState::update() {
    inputController.update(); 
    ledController.update(); // For any LED animations if used (like flash)
    // No specific logic needed in update for DoneState as it's event-driven to exit
}

void DoneState::exit() {
    Serial.println("Exiting Done State");
    lv_obj_t *screen = lv_screen_active();
    if (screen) {
        lv_obj_remove_event_cb_with_user_data(screen, done_screen_tap_event_handler, this);
        lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    }
    // LVGL objects are children of the screen, lv_obj_clean in enter() handles old ones.
    doneLabel = nullptr;
    timeDisplayLabel = nullptr;
    projectNameLabel = nullptr;
    instructionLabel = nullptr;

    // Optional: Clear pending project ID from state machine if not already done in processScreenTap
    // stateMachine.clearPendingProject(); 
}
