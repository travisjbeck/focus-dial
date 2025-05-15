#include "states/ProjectSelectState.h"
#include "StateMachine.h"
#include "Controllers.h"
#include <lvgl.h> // Ensure LVGL is included

#define PROJECT_SELECT_TIMEOUT 30000 // 30 seconds

// Static LVGL event handlers
static void roller_event_handler(lv_event_t *e);
static void screen_tap_event_handler(lv_event_t *e);
static void screen_long_press_event_handler(lv_event_t *e);

// --- Public Helper Methods for LVGL Event Callbacks ---
void ProjectSelectState::processRollerValueChange(lv_obj_t* roller_obj) {
    selectedProjectIndex = lv_roller_get_selected(roller_obj);
    Serial.printf("ProjectSelectState: Roller new index: %d\n", selectedProjectIndex);
    updateLedColor();
    lastActivityTime = millis();
}

void ProjectSelectState::processScreenTap() {
    Serial.println("ProjectSelectState: Screen tapped - Confirming project");
    
    int duration = stateMachine.getPendingDuration();
    int indexToSave = (selectedProjectIndex == 0) ? -1 : selectedProjectIndex - 1;
    projectManager.setLastProjectIndex(indexToSave);
    Serial.printf("Selected project index %d (saved as %d)\n", selectedProjectIndex, indexToSave);
    
    String selectedProjectId = "";
    if (selectedProjectIndex > 0 && selectedProjectIndex < projectsWithNone.size()) {
        selectedProjectId = projectsWithNone[selectedProjectIndex].device_project_id;
    }
    Serial.printf("Selected device_project_id: %s\n", selectedProjectId.c_str());
    stateMachine.setPendingProjectId(selectedProjectId);
    
    StateMachine::timerState.setTimer(duration, 0);
    stateMachine.changeState(&StateMachine::timerState);
}

void ProjectSelectState::processScreenLongPress() {
    Serial.println("ProjectSelectState: Screen long pressed - Going back to Idle");
    stateMachine.changeState(&StateMachine::idleState);
}

// --- Static LVGL Event Handlers (calling public helpers) ---
static void roller_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    ProjectSelectState* self = (ProjectSelectState*)lv_event_get_user_data(e);
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e)); 

    if (code == LV_EVENT_VALUE_CHANGED && self && obj) {
        self->processRollerValueChange(obj);
    }
}

static void screen_tap_event_handler(lv_event_t *e) {
    ProjectSelectState* self = (ProjectSelectState*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED && self) {
        self->processScreenTap();
    }
}

static void screen_long_press_event_handler(lv_event_t *e) {
    ProjectSelectState* self = (ProjectSelectState*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_LONG_PRESSED && self) {
        self->processScreenLongPress();
    }
}

ProjectSelectState::ProjectSelectState(StateMachine &sm, DisplayController &display, LEDController &leds, InputController &input, ProjectManager &pm)
    : stateMachine(sm),
      displayController(display),
      ledController(leds),
      inputController(input),
      projectManager(pm),
      selectedProjectIndex(0),
      titleLabel(nullptr),
      roller(nullptr),
      needsInitialRender(true),
      lastActivityTime(0)
{
}

void ProjectSelectState::enter()
{
  Serial.println("Entering Project Select State");

  loadProjects(); 

  int lastUsedIndex = projectManager.getLastProjectIndex();
  selectedProjectIndex = (lastUsedIndex >= 0 && (lastUsedIndex + 1) < projectsWithNone.size()) ? (lastUsedIndex + 1) : 0;
  Serial.printf("Initial selected index: %d\n", selectedProjectIndex);

  lv_obj_t *screen = lv_screen_active();
  if (!screen) {
      Serial.println("ProjectSelectState::enter() - FATAL: lv_screen_active() returned NULL! Cannot create UI.");
      return;
  }
  lv_obj_clean(screen); 

  titleLabel = lv_label_create(screen);
  lv_label_set_text(titleLabel, "Select Project");
  lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);

  roller = lv_roller_create(screen);
  String roller_options = "";
  for (size_t i = 0; i < projectsWithNone.size(); ++i) {
    roller_options += projectsWithNone[i].name;
    if (i < projectsWithNone.size() - 1) {
      roller_options += "\n";
    }
  }
  lv_roller_set_options(roller, roller_options.c_str(), LV_ROLLER_MODE_NORMAL);
  lv_roller_set_visible_row_count(roller, 3);
  lv_obj_set_width(roller, lv_pct(80));
  lv_obj_align(roller, LV_ALIGN_CENTER, 0, 0);
  lv_roller_set_selected(roller, selectedProjectIndex, LV_ANIM_OFF);
  lv_obj_add_event_cb(roller, roller_event_handler, LV_EVENT_VALUE_CHANGED, this);

  lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE); 
  lv_obj_add_event_cb(screen, screen_tap_event_handler, LV_EVENT_CLICKED, this);
  Serial.println("ProjectSelectState: Full screen tap event added.");
  lv_obj_add_event_cb(screen, screen_long_press_event_handler, LV_EVENT_LONG_PRESSED, this);
  Serial.println("ProjectSelectState: Full screen long press event added.");

  handleInput(); 
  updateLedColor();
  lastActivityTime = millis();
  needsInitialRender = false; 
  lv_refr_now(NULL); 
}

void ProjectSelectState::update()
{
  inputController.update(); 
  ledController.update();

  if (millis() - lastActivityTime >= PROJECT_SELECT_TIMEOUT)
  {
    Serial.println("ProjectSelectState: Timeout - Returning to Idle");
    stateMachine.changeState(&StateMachine::idleState);
  }
}

void ProjectSelectState::exit()
{
  Serial.println("Exiting Project Select State");
  inputController.releaseHandlers(); 
  ledController.turnOff(); 

  lv_obj_t *screen = lv_screen_active();
  if (screen) {
      lv_obj_remove_event_cb_with_user_data(screen, screen_tap_event_handler, this);
      lv_obj_remove_event_cb_with_user_data(screen, screen_long_press_event_handler, this);
      lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE); 
      Serial.println("ProjectSelectState: Full screen tap and long press events removed.");
  }

  if (titleLabel) {
    lv_obj_del(titleLabel);
    titleLabel = nullptr;
  }
  if (roller) {
    lv_obj_del(roller);
    roller = nullptr;
  }
  Serial.println("ProjectSelectState: LVGL objects cleaned.");
}

// --- Helper Methods ---

void ProjectSelectState::renderDisplay()
{
  // This method might become obsolete or be used for minor updates if needed.
  // For now, main drawing is in enter() and event handlers.
  updateLedColor();
}

void ProjectSelectState::updateLedColor()
{
  // Get color from projectsWithNone[selectedProjectIndex]
  // Convert hex to uint32_t using LEDController::hexColorToUint32
  // Set LED color using ledController.setSolid() or similar
  if (selectedProjectIndex >= 0 && selectedProjectIndex < projectsWithNone.size())
  {
    uint32_t color = LEDController::hexColorToUint32(projectsWithNone[selectedProjectIndex].color);
    ledController.setSolid(color);
  }
  else
  {
    ledController.turnOff(); // Should not happen
  }
}

void ProjectSelectState::loadProjects()
{
  projectsWithNone.clear();
  Project noProject = {"No Project", "#FF0000"}; // Red for no project
  projectsWithNone.push_back(noProject);
  const auto &actualProjects = projectManager.getProjects();
  projectsWithNone.insert(projectsWithNone.end(), actualProjects.begin(), actualProjects.end());
}

void ProjectSelectState::handleInput()
{
  inputController.onEncoderRotateHandler([this](int delta)
                                         {
                                           if (!roller) return;
                                           int current_sel = lv_roller_get_selected(roller);
                                           int listSize = projectsWithNone.size();
                                           if (listSize == 0) return;

                                           current_sel += delta;
                                           current_sel = (current_sel % listSize + listSize) % listSize; 
                                           
                                           lv_roller_set_selected(roller, current_sel, LV_ANIM_ON);
                                           // The LV_EVENT_VALUE_CHANGED on the roller will handle updating selectedProjectIndex and LED color
                                           
                                           Serial.printf("ProjectSelectState: Encoder Delta: %d, Roller new sel: %d\n", delta, current_sel);
                                           lastActivityTime = millis(); 
                                         });
}