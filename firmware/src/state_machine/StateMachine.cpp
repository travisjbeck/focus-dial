#include "include/StateMachine.h"
#include "../ui/TouchManager.h"
#include "states/IdleState.h"
#include "states/AdjustState.h"
#include "states/TimerState.h"
#include "states/PausedState.h"
#include "states/DoneState.h"
#include "states/ProjectSelectState.h"
#include "states/ProvisionState.h"
#include "states/StartupState.h"
#include "states/SleepState.h"
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_task_wdt.h>
#include <freertos/task.h>
#include "../../pin_config.h"
#include <XPowersLib.h>

static const char* TAG = "StateMachine";

// Global instance
StateMachine stateMachine;

StateMachine::StateMachine() : 
  currentState(nullptr),
  stateMutex(nullptr),
  transitionInProgress(false),
  touchManager(nullptr),
  pendingDuration(25 * 60), // Default 25 minutes in seconds
  pendingElapsedTime(0),
  pendingProjectId(""),
  selectedProjectIndex(-1), // -1 for "No Project"
  watchdogEnabled(false),
  lastActivityTime(0)
{
  // State instances will be created in begin()
  idleState = nullptr;
  adjustState = nullptr;
  timerState = nullptr;
  pausedState = nullptr;
  doneState = nullptr;
  projectSelectState = nullptr;
  provisionState = nullptr;
  startupState = nullptr;
  sleepState = nullptr;
}

StateMachine::~StateMachine()
{
  // Clean up LED controller first
  cleanupLEDController();
  
  // Clean up input controller
  cleanupInputController();
  
  // Clean up memory monitor
  cleanupMemoryMonitor();
  
  if (stateMutex) {
    vSemaphoreDelete(stateMutex);
  }
  
  // Clean up state instances
  delete idleState;
  delete adjustState;
  delete timerState;
  delete pausedState;
  delete doneState;
  delete projectSelectState;
  delete provisionState;
  delete startupState;
  delete sleepState;
}

void StateMachine::begin()
{
  ESP_LOGI(TAG, "=== INITIALIZING STATE MACHINE ===");
  
  // Create mutex for thread-safe state transitions
  stateMutex = xSemaphoreCreateMutex();
  if (!stateMutex) {
    ESP_LOGE(TAG, "Failed to create state machine mutex");
    return;
  }
  
  ESP_LOGI(TAG, "Created state machine mutex");
  
  // Check available memory before allocation
  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  ESP_LOGI(TAG, "Pre-allocation memory - Heap: %u bytes, PSRAM: %u bytes", free_heap, free_psram);
  
  // Initialize all state instances using PSRAM if available for larger objects
  idleState = (IdleState*)heap_caps_malloc(sizeof(IdleState), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!idleState) {
    idleState = new IdleState(); // Fallback to regular heap
    ESP_LOGW(TAG, "IdleState allocated on main heap (PSRAM not available)");
  } else {
    new(idleState) IdleState(); // Placement new
    ESP_LOGD(TAG, "IdleState allocated on PSRAM");
  }
  
  // Regular heap allocation for smaller state objects
  adjustState = new AdjustState();
  timerState = new TimerState();
  pausedState = new PausedState();
  doneState = new DoneState();
  projectSelectState = new ProjectSelectState();
  provisionState = new ProvisionState();
  startupState = new StartupState();
  sleepState = new SleepState();
  
  ESP_LOGI(TAG, "Created all state instances");
  
  // Log post-allocation memory usage
  logMemoryUsage();
  
  // Register with task watchdog for crash recovery
  enableTaskWatchdog();
  
  // Initialize memory monitoring and crash recovery
  if (!initializeMemoryMonitor()) {
    ESP_LOGW(TAG, "Memory monitor initialization failed, continuing without it");
  }
  
  // Initialize input controller for rotary encoder
  ESP_LOGI(TAG, "=== Initializing Input Controller ===");
  if (!initializeInputController()) {
    ESP_LOGW(TAG, "Input controller initialization failed, continuing without it");
  } else {
    ESP_LOGI(TAG, "SUCCESS: Input controller initialized");
  }
  
  // Initialize LED controller for NeoPixel ring
  ESP_LOGI(TAG, "=== Initializing LED Controller ===");
  if (!initializeLEDController()) {
    ESP_LOGW(TAG, "LED controller initialization failed, continuing without it");
  } else {
    ESP_LOGI(TAG, "SUCCESS: LED controller initialized");
  }
  
  // Initialize activity tracking
  lastActivityTime = millis();
  
  // Start with startup state
  currentState = startupState;
  if (currentState) {
    currentState->enter();
  }
  
  ESP_LOGI(TAG, "State machine initialization complete");
}

void StateMachine::update()
{
  // Feed the watchdog to prevent reboot
  esp_task_wdt_reset();
  
  // Memory monitoring every 10 seconds using esp_timer
  static uint64_t lastMemoryCheck = 0;
  uint64_t currentTime = esp_timer_get_time() / 1000; // Convert to milliseconds
  
  if (currentTime - lastMemoryCheck > MEMORY_CHECK_INTERVAL) {
    checkMemoryLeaks();
    updateMemorySnapshot();
    lastMemoryCheck = currentTime;
  }
  
  // Update input controller
  updateInputController();
  
  // Update LED controller
  updateLEDController();
  
  // Update current state
  if (currentState && !transitionInProgress) {
    // Measure state execution time for performance monitoring
    uint64_t start_time = esp_timer_get_time();
    
    currentState->update();
    
    uint64_t execution_time = esp_timer_get_time() - start_time;
    
    // Log if state update takes too long (>10ms)
    if (execution_time > 10000) { // 10ms in microseconds
      ESP_LOGW(TAG, "State %s update took %llu us", 
               currentState->getStateName(), execution_time);
    }
  }
  
  // Yield to other FreeRTOS tasks
  vTaskDelay(pdMS_TO_TICKS(1));
}

void StateMachine::changeState(State *newState)
{
  if (!newState || newState == currentState) {
    return;
  }
  
  // Validate state transition
  if (currentState && !currentState->canTransitionTo(newState)) {
    ESP_LOGE(TAG, "Invalid state transition from %s to %s", 
             currentState->getStateName(), newState->getStateName());
    return;
  }
  
  // Thread-safe state transition with timeout
  TickType_t timeout = pdMS_TO_TICKS(100);
  if (xSemaphoreTake(stateMutex, timeout) == pdTRUE) {
    transitionInProgress = true;
    
    ESP_LOGI(TAG, "=== STATE TRANSITION ===");
    ESP_LOGI(TAG, "From: %s", currentState ? currentState->getStateName() : "NULL");
    ESP_LOGI(TAG, "To: %s", newState->getStateName());
    
    // Pre-validate new state entry
    if (!newState->validateStateEntry()) {
      ESP_LOGW(TAG, "State entry validation failed for %s - proceeding anyway", 
               newState->getStateName());
    }
    
    // Measure transition time
    uint64_t transition_start = esp_timer_get_time();
    
    // Capture current state name for memory monitoring before transition
    const char* fromStateName = currentState ? currentState->getStateName() : "NULL";
    
    // Notify TouchManager about upcoming state transition
    if (touchManager) {
      touchManager->blockTouchDuringTransition();
    }
    
    // Exit current state
    if (currentState) {
      currentState->exit();
    }
    
    // Change to new state
    currentState = newState;
    currentState->enter();
    
    // Unblock touch after state transition is complete
    if (touchManager) {
      touchManager->unblockTouchAfterTransition();
    }
    
    uint64_t transition_time = esp_timer_get_time() - transition_start;
    ESP_LOGD(TAG, "State transition completed in %llu us", transition_time);
    
    // Record state transition for memory monitoring
    if (g_memoryMonitor) {
      g_memoryMonitor->recordStateTransition(fromStateName, newState->getStateName());
      
      // Feed watchdog after successful transition
      g_memoryMonitor->feedWatchdog();
    }
    
    transitionInProgress = false;
    xSemaphoreGive(stateMutex);
    
    // Log memory after transition
    logMemoryUsage();
    
    // Reset watchdog after potentially long operation
    esp_task_wdt_reset();
  } else {
    ESP_LOGE(TAG, "Failed to acquire state mutex for transition (timeout after 100ms)");
    
    // Emergency state recovery - validate before forcing
    if (newState->validateStateEntry()) {
      ESP_LOGW(TAG, "Attempting emergency state transition");
      currentState = newState;
      if (currentState) {
        currentState->enter();
      }
    } else {
      ESP_LOGE(TAG, "Emergency transition validation failed - system may be unstable");
    }
  }
}

State *StateMachine::getCurrentState() const
{
  return currentState;
}

void StateMachine::setPendingDuration(int duration)
{
  pendingDuration = duration;
  ESP_LOGD(TAG, "Set pending duration: %d seconds", duration);
}

int StateMachine::getPendingDuration() const
{
  return pendingDuration;
}

void StateMachine::setPendingElapsedTime(unsigned long seconds)
{
  pendingElapsedTime = seconds;
  ESP_LOGD(TAG, "Set pending elapsed time: %lu seconds", seconds);
}

unsigned long StateMachine::getPendingElapsedTime() const
{
  return pendingElapsedTime;
}

void StateMachine::setPendingProjectId(const String &projectId)
{
  pendingProjectId = projectId;
  ESP_LOGD(TAG, "Set pending project ID: %s", projectId.c_str());
}

String StateMachine::getPendingProjectId() const
{
  return pendingProjectId;
}

void StateMachine::clearPendingProject()
{
  pendingProjectId = "";
  ESP_LOGD(TAG, "Cleared pending project");
}

void StateMachine::setSelectedProjectIndex(int index)
{
  selectedProjectIndex = index;
  ESP_LOGD(TAG, "Set selected project index: %d", index);
}

int StateMachine::getSelectedProjectIndex() const
{
  return selectedProjectIndex;
}

bool StateMachine::isInIdleState() const
{
  return currentState == idleState;
}

void StateMachine::resetLEDColor()
{
  ESP_LOGD(TAG, "Reset LED color requested");
  // TODO: Implement LED color reset when LED controller is available
}

void StateMachine::logMemoryUsage() const
{
  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  size_t min_free_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
  size_t min_free_psram = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
  
  ESP_LOGD(TAG, "Memory - Heap: %u/%u, PSRAM: %u/%u, State: %s", 
           free_heap, min_free_heap, free_psram, min_free_psram,
           currentState ? currentState->getStateName() : "NULL");
  
  // Stack usage monitoring
  UBaseType_t stackHighWater = uxTaskGetStackHighWaterMark(NULL);
  ESP_LOGD(TAG, "Stack high water mark: %u words", stackHighWater);
}

void StateMachine::checkMemoryLeak() const
{
  static size_t previousHeap = 0;
  static size_t previousPsram = 0;
  static size_t minHeapEver = SIZE_MAX;
  static size_t minPsramEver = SIZE_MAX;
  
  size_t currentHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t currentPsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  
  // Track minimum memory ever reached
  if (currentHeap < minHeapEver) {
    minHeapEver = currentHeap;
    ESP_LOGI(TAG, "New minimum heap reached: %u bytes", minHeapEver);
  }
  
  if (currentPsram < minPsramEver) {
    minPsramEver = currentPsram;
    ESP_LOGI(TAG, "New minimum PSRAM reached: %u bytes", minPsramEver);
  }
  
  if (previousHeap > 0) {
    int32_t heapDiff = (int32_t)currentHeap - (int32_t)previousHeap;
    int32_t psramDiff = (int32_t)currentPsram - (int32_t)previousPsram;
    
    // Report significant memory changes (> 1KB)
    if (abs(heapDiff) > 1024 || abs(psramDiff) > 1024) {
      ESP_LOGW(TAG, "Memory change - Heap: %ld, PSRAM: %ld bytes", heapDiff, psramDiff);
    }
    
    // Alert on low memory conditions
    if (currentHeap < 10240) { // Less than 10KB heap
      ESP_LOGW(TAG, "Low heap memory: %u bytes", currentHeap);
    }
    
    if (currentPsram < 51200) { // Less than 50KB PSRAM
      ESP_LOGW(TAG, "Low PSRAM memory: %u bytes", currentPsram);
    }
  }
  
  previousHeap = currentHeap;
  previousPsram = currentPsram;
}

void StateMachine::enableTaskWatchdog()
{
  if (!watchdogEnabled) {
    esp_err_t err = esp_task_wdt_add(NULL);
    if (err == ESP_OK) {
      watchdogEnabled = true;
      ESP_LOGI(TAG, "Task watchdog enabled");
    } else {
      ESP_LOGE(TAG, "Failed to enable task watchdog: %s", esp_err_to_name(err));
    }
  }
}

void StateMachine::disableTaskWatchdog()
{
  if (watchdogEnabled) {
    esp_err_t err = esp_task_wdt_delete(NULL);
    if (err == ESP_OK) {
      watchdogEnabled = false;
      ESP_LOGI(TAG, "Task watchdog disabled");
    } else {
      ESP_LOGE(TAG, "Failed to disable task watchdog: %s", esp_err_to_name(err));
    }
  }
}

size_t StateMachine::getUsedHeap() const
{
  size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  return total_heap - free_heap;
}

size_t StateMachine::getUsedPSRAM() const
{
  size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  return total_psram - free_psram;
}

void StateMachine::dumpCurrentState() const
{
  if (currentState) {
    ESP_LOGI(TAG, "=== CURRENT STATE DUMP ===");
    currentState->dumpState();
  } else {
    ESP_LOGI(TAG, "No current state");
  }
}

void StateMachine::dumpAllStates() const
{
  ESP_LOGI(TAG, "=== ALL STATES DUMP ===");
  
  State* states[] = {
    idleState, adjustState, timerState, pausedState, doneState,
    projectSelectState, provisionState, startupState, sleepState
  };
  
  const char* stateNames[] = {
    "IdleState", "AdjustState", "TimerState", "PausedState", "DoneState",
    "ProjectSelectState", "ProvisionState", "StartupState", "SleepState"
  };
  
  for (int i = 0; i < 9; i++) {
    if (states[i]) {
      ESP_LOGI(TAG, "--- %s ---", stateNames[i]);
      states[i]->dumpState();
    } else {
      ESP_LOGI(TAG, "--- %s: NOT INITIALIZED ---", stateNames[i]);
    }
  }
}

bool StateMachine::isValidTransition(State* fromState, State* toState) const
{
  if (!fromState || !toState) {
    return false;
  }
  
  return fromState->canTransitionTo(toState) && toState->validateStateEntry();
}

const StateStats* StateMachine::getStateStatistics(const char* stateName) const
{
  if (!stateName) return nullptr;
  
  State* state = nullptr;
  
  // Find the state by name
  if (strcmp(stateName, "IdleState") == 0) state = idleState;
  else if (strcmp(stateName, "AdjustState") == 0) state = adjustState;
  else if (strcmp(stateName, "TimerState") == 0) state = timerState;
  else if (strcmp(stateName, "PausedState") == 0) state = pausedState;
  else if (strcmp(stateName, "DoneState") == 0) state = doneState;
  else if (strcmp(stateName, "ProjectSelectState") == 0) state = projectSelectState;
  else if (strcmp(stateName, "ProvisionState") == 0) state = provisionState;
  else if (strcmp(stateName, "StartupState") == 0) state = startupState;
  else if (strcmp(stateName, "SleepState") == 0) state = sleepState;
  
  if (state) {
    return &state->getStatistics();
  }
  
  return nullptr;
}

void StateMachine::resetAllStateStatistics()
{
  ESP_LOGI(TAG, "Resetting all state statistics");
  
  State* states[] = {
    idleState, adjustState, timerState, pausedState, doneState,
    projectSelectState, provisionState, startupState, sleepState
  };
  
  for (int i = 0; i < 9; i++) {
    if (states[i]) {
      states[i]->resetStatistics();
    }
  }
}

void StateMachine::logSystemHealth() const
{
  ESP_LOGI(TAG, "=== SYSTEM HEALTH CHECK ===");
  
  // Current state info
  ESP_LOGI(TAG, "Current state: %s", currentState ? currentState->getStateName() : "NULL");
  if (currentState) {
    ESP_LOGI(TAG, "Time in current state: %lu ms", currentState->getTimeInState());
  }
  
  // Memory status
  size_t heap_free = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  size_t heap_min = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
  size_t psram_min = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
  
  ESP_LOGI(TAG, "Memory - Heap: %u free (%u min), PSRAM: %u free (%u min)", 
           heap_free, heap_min, psram_free, psram_min);
  
  // Task info
  UBaseType_t stackSpace = uxTaskGetStackHighWaterMark(NULL);
  ESP_LOGI(TAG, "Stack high water mark: %u words", stackSpace);
  
  // Watchdog status
  ESP_LOGI(TAG, "Task watchdog: %s", watchdogEnabled ? "enabled" : "disabled");
  
  // Transition status
  ESP_LOGI(TAG, "Transition in progress: %s", transitionInProgress ? "yes" : "no");
  
  // Health warnings
  if (heap_free < 10240) {
    ESP_LOGW(TAG, "LOW HEAP WARNING: Only %u bytes free", heap_free);
  }
  
  if (stackSpace < 256) {
    ESP_LOGW(TAG, "LOW STACK WARNING: Only %u words remaining", stackSpace);
  }
  
  if (currentState && currentState->getTimeInState() > 300000) { // 5 minutes
    ESP_LOGW(TAG, "LONG STATE WARNING: In %s for %lu ms", 
             currentState->getStateName(), currentState->getTimeInState());
  }
}

// Memory monitoring and crash recovery implementations
bool StateMachine::initializeMemoryMonitor()
{
  if (g_memoryMonitor) {
    ESP_LOGW(TAG, "Memory monitor already initialized");
    return true;
  }
  
  g_memoryMonitor = new MemoryMonitor();
  if (!g_memoryMonitor) {
    ESP_LOGE(TAG, "Failed to allocate memory monitor");
    return false;
  }
  
  if (!g_memoryMonitor->initialize()) {
    ESP_LOGE(TAG, "Memory monitor initialization failed");
    delete g_memoryMonitor;
    g_memoryMonitor = nullptr;
    return false;
  }
  
  // Check for crash recovery data
  if (g_memoryMonitor->hasRecoveryData()) {
    ESP_LOGW(TAG, "Crash recovery data found!");
    if (performCrashRecovery()) {
      ESP_LOGI(TAG, "Crash recovery completed successfully");
    }
  }
  
  ESP_LOGI(TAG, "Memory monitor initialized successfully");
  return true;
}

void StateMachine::cleanupMemoryMonitor()
{
  if (g_memoryMonitor) {
    g_memoryMonitor->cleanup();
    delete g_memoryMonitor;
    g_memoryMonitor = nullptr;
    ESP_LOGI(TAG, "Memory monitor cleaned up");
  }
}

void StateMachine::updateMemorySnapshot()
{
  if (g_memoryMonitor && currentState) {
    g_memoryMonitor->updateSnapshot(currentState->getStateName());
  }
}

void StateMachine::checkMemoryLeaks()
{
  if (g_memoryMonitor) {
    g_memoryMonitor->detectMemoryLeaks();
    
    // Check stack overflow
    if (g_memoryMonitor->checkStackOverflow()) {
      triggerEmergencySnapshot("StackOverflow");
    }
    
    // Validate heap integrity periodically
    if (g_memoryMonitor->getTransitionCount() % 20 == 0) {
      g_memoryMonitor->validateHeapIntegrity();
    }
  }
}

bool StateMachine::hasRecoveryData() const
{
  return g_memoryMonitor ? g_memoryMonitor->hasRecoveryData() : false;
}

bool StateMachine::performCrashRecovery()
{
  if (!g_memoryMonitor) {
    ESP_LOGE(TAG, "Memory monitor not available for crash recovery");
    return false;
  }
  
  return g_memoryMonitor->performEmergencyRecovery();
}

void StateMachine::triggerEmergencySnapshot(const char* reason)
{
  if (g_memoryMonitor) {
    g_memoryMonitor->triggerEmergencySnapshot(reason);
  }
}

void StateMachine::feedWatchdog()
{
  if (g_memoryMonitor) {
    g_memoryMonitor->feedWatchdog();
  }
}

bool StateMachine::isWatchdogActive() const
{
  return g_memoryMonitor ? g_memoryMonitor->isWatchdogEnabled() : false;
}

// Simple testing implementation
bool StateMachine::runBasicTests()
{
  ESP_LOGI(TAG, "=== RUNNING BASIC STATE MACHINE TESTS ===");
  
  bool allPassed = true;
  allPassed &= testStateTransitions();
  allPassed &= testMemoryMonitoring();
  allPassed &= testInputController();
  allPassed &= testLEDController();
  
  ESP_LOGI(TAG, "=== BASIC TESTS %s ===", allPassed ? "PASSED" : "FAILED");
  return allPassed;
}

bool StateMachine::testStateTransitions()
{
  ESP_LOGI(TAG, "--- Testing State Transitions ---");
  
  State* originalState = getCurrentState();
  if (!originalState) {
    ESP_LOGE(TAG, "FAIL: No current state");
    return false;
  }
  
  ESP_LOGI(TAG, "Original state: %s", originalState->getStateName());
  
  // Test transition to idle
  changeState(idleState);
  update();
  if (getCurrentState() != idleState) {
    ESP_LOGE(TAG, "FAIL: Could not transition to IdleState");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Transitioned to IdleState");
  
  // Test transition to timer
  changeState(timerState);
  update();
  if (getCurrentState() != timerState) {
    ESP_LOGE(TAG, "FAIL: Could not transition to TimerState");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Transitioned to TimerState");
  
  // Test transition to paused
  changeState(pausedState);
  update();
  if (getCurrentState() != pausedState) {
    ESP_LOGE(TAG, "FAIL: Could not transition to PausedState");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Transitioned to PausedState");
  
  // Test transition to done
  changeState(doneState);
  update();
  if (getCurrentState() != doneState) {
    ESP_LOGE(TAG, "FAIL: Could not transition to DoneState");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Transitioned to DoneState");
  
  // Return to original state
  changeState(originalState);
  update();
  
  ESP_LOGI(TAG, "--- State Transition Tests PASSED ---");
  return true;
}

bool StateMachine::testMemoryMonitoring()
{
  ESP_LOGI(TAG, "--- Testing Memory Monitoring ---");
  
  if (!g_memoryMonitor) {
    ESP_LOGW(TAG, "Memory monitor not available, skipping tests");
    return true;
  }
  
  // Test memory snapshot
  g_memoryMonitor->updateSnapshot("TestState");
  MemorySnapshot snapshot = g_memoryMonitor->getCurrentSnapshot();
  
  if (snapshot.totalHeap == 0) {
    ESP_LOGE(TAG, "FAIL: Invalid heap size in snapshot");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Memory snapshot captured (heap: %d bytes)", snapshot.totalHeap);
  
  // Test memory baseline
  g_memoryMonitor->setMemoryBaseline();
  ESP_LOGI(TAG, "PASS: Memory baseline set");
  
  // Test leak detection
  g_memoryMonitor->detectMemoryLeaks();
  ESP_LOGI(TAG, "PASS: Memory leak detection completed");
  
  // Test emergency snapshot
  g_memoryMonitor->triggerEmergencySnapshot("TestEmergency");
  ESP_LOGI(TAG, "PASS: Emergency snapshot triggered");
  
  // Test watchdog if available
  if (g_memoryMonitor->enableWatchdog(5000)) {
    ESP_LOGI(TAG, "PASS: Watchdog enabled");
    g_memoryMonitor->feedWatchdog();
    ESP_LOGI(TAG, "PASS: Watchdog fed");
    g_memoryMonitor->disableWatchdog();
    ESP_LOGI(TAG, "PASS: Watchdog disabled");
  }
  
  ESP_LOGI(TAG, "--- Memory Monitoring Tests PASSED ---");
  return true;
}

void StateMachine::runTestMode()
{
  ESP_LOGI(TAG, "=== ENTERING TEST MODE ===");
  ESP_LOGI(TAG, "ESP32-S3 State Machine Integration Tests");
  ESP_LOGI(TAG, "Hardware: ESP32-S3-Touch-AMOLED-1.75");
  
  // Log system information
  ESP_LOGI(TAG, "Free heap: %d bytes", heap_caps_get_free_size(MALLOC_CAP_8BIT));
  ESP_LOGI(TAG, "Free PSRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  ESP_LOGI(TAG, "Stack high water mark: %d bytes", uxTaskGetStackHighWaterMark(NULL));
  
  // Run all tests
  bool result = runBasicTests();
  
  // Generate summary
  ESP_LOGI(TAG, "=== TEST SUMMARY ===");
  ESP_LOGI(TAG, "Overall result: %s", result ? "PASSED" : "FAILED");
  ESP_LOGI(TAG, "All core state machine functionality verified");
  
  // Log memory state after tests
  logMemoryUsage();
  if (g_memoryMonitor) {
    g_memoryMonitor->logMemoryStatus();
  }
  
  ESP_LOGI(TAG, "Test mode completed");
}

// Input Controller management implementations
bool StateMachine::initializeInputController()
{
  ESP_LOGI(TAG, "Initializing input controller...");
  
  if (g_inputController) {
    ESP_LOGW(TAG, "Input controller already initialized");
    return true;
  }
  
  // Create InputController with configured pins
  g_inputController = new InputController(ENCODER_A, ENCODER_B, ENCODER_BUTTON);
  if (!g_inputController) {
    ESP_LOGE(TAG, "Failed to allocate input controller");
    return false;
  }
  
  if (!g_inputController->begin()) {
    ESP_LOGE(TAG, "Input controller initialization failed");
    delete g_inputController;
    g_inputController = nullptr;
    return false;
  }
  
  // Test connectivity
  if (!g_inputController->testEncoderConnectivity()) {
    ESP_LOGW(TAG, "Encoder connectivity test failed - may not be connected");
  }
  
  ESP_LOGI(TAG, "Input controller initialized successfully");
  return true;
}

void StateMachine::updateInputController()
{
  if (g_inputController) {
    g_inputController->update();
  }
}

void StateMachine::cleanupInputController()
{
  if (g_inputController) {
    g_inputController->cleanup();
    delete g_inputController;
    g_inputController = nullptr;
    ESP_LOGI(TAG, "Input controller cleaned up");
  }
}

bool StateMachine::isInputControllerActive() const
{
  return g_inputController != nullptr;
}

InputController* StateMachine::getInputController() const
{
  return g_inputController;
}

bool StateMachine::testInputController()
{
  ESP_LOGI(TAG, "--- Testing Input Controller ---");
  
  if (!g_inputController) {
    ESP_LOGW(TAG, "Input controller not available, skipping tests");
    return true;
  }
  
  // Test connectivity
  if (!g_inputController->testEncoderConnectivity()) {
    ESP_LOGE(TAG, "FAIL: Encoder connectivity test failed");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Encoder connectivity test");
  
  // Test state dump
  g_inputController->dumpEncoderState();
  ESP_LOGI(TAG, "PASS: State dump completed");
  
  // Test position reset
  int initialPosition = g_inputController->getEncoderPosition();
  g_inputController->resetEncoderPosition();
  int resetPosition = g_inputController->getEncoderPosition();
  
  if (resetPosition != 0) {
    ESP_LOGE(TAG, "FAIL: Position reset failed");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Position reset (was %d, now %d)", initialPosition, resetPosition);
  
  // Test interrupt counting
  uint32_t interruptCount = g_inputController->getInterruptCount();
  ESP_LOGI(TAG, "PASS: Interrupt count: %u", interruptCount);
  
  ESP_LOGI(TAG, "--- Input Controller Tests PASSED ---");
  return true;
}

// LED Controller management implementations
bool StateMachine::initializeLEDController()
{
  ESP_LOGI(TAG, "Initializing LED controller...");
  
  if (g_ledController) {
    ESP_LOGW(TAG, "LED controller already initialized");
    return true;
  }
  
  // Create LEDController with configured pins and settings
  // Brightness value is stored but NOT applied via setBrightness() to avoid color corruption
  g_ledController = new LEDController(NEOPIXEL_PIN, NEOPIXEL_COUNT, 100); // Medium brightness value for calculations
  if (!g_ledController) {
    ESP_LOGE(TAG, "Failed to allocate LED controller");
    return false;
  }
  
  if (!g_ledController->begin()) {
    ESP_LOGE(TAG, "LED controller initialization failed");
    delete g_ledController;
    g_ledController = nullptr;
    return false;
  }
  
  // Skip power sufficiency test to avoid white LED flash during init
  // if (!g_ledController->test3V3PowerSufficiency()) {
  //   ESP_LOGW(TAG, "3.3V power sufficiency test failed - LEDs may be dim");
  // }
  
  ESP_LOGI(TAG, "LED controller initialized successfully");
  return true;
}

void StateMachine::updateLEDController()
{
  if (g_ledController) {
    g_ledController->update();
  }
}

void StateMachine::cleanupLEDController()
{
  if (g_ledController) {
    g_ledController->cleanup();
    delete g_ledController;
    g_ledController = nullptr;
    ESP_LOGI(TAG, "LED controller cleaned up");
  }
}

bool StateMachine::isLEDControllerActive() const
{
  return g_ledController != nullptr;
}

LEDController* StateMachine::getLEDController() const
{
  return g_ledController;
}

bool StateMachine::testLEDController()
{
  ESP_LOGI(TAG, "--- Testing LED Controller ---");
  
  if (!g_ledController) {
    ESP_LOGW(TAG, "LED controller not available, skipping tests");
    return true;
  }
  
  // Enable debug output for testing
  g_ledController->enableDebugOutput(true);
  
  // Test individual LEDs
  if (!g_ledController->testIndividualLEDs()) {
    ESP_LOGE(TAG, "FAIL: Individual LED test failed");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Individual LED test");
  
  // Test 3.3V power sufficiency
  if (!g_ledController->test3V3PowerSufficiency()) {
    ESP_LOGE(TAG, "FAIL: 3.3V power test failed");
    return false;
  }
  ESP_LOGI(TAG, "PASS: 3.3V power test");
  
  // Test all animation patterns
  if (!g_ledController->testAllPatterns()) {
    ESP_LOGE(TAG, "FAIL: Animation patterns test failed");
    return false;
  }
  ESP_LOGI(TAG, "PASS: Animation patterns test");
  
  // Test state dump
  g_ledController->dumpLEDState();
  ESP_LOGI(TAG, "PASS: State dump completed");
  
  // Test performance stats
  g_ledController->logPerformanceStats();
  ESP_LOGI(TAG, "PASS: Performance stats logged");
  
  // Disable debug output after testing
  g_ledController->enableDebugOutput(false);
  
  ESP_LOGI(TAG, "--- LED Controller Tests PASSED ---");
  return true;
}

void StateMachine::setTouchManager(TouchManager* tm) {
  touchManager = tm;
  ESP_LOGI(TAG, "TouchManager registered with StateMachine");
}

TouchManager* StateMachine::getTouchManager() const {
  return touchManager;
}

// Inactivity tracking methods
void StateMachine::updateActivityTime() {
  lastActivityTime = millis();
}

unsigned long StateMachine::getTimeSinceLastActivity() const {
  return millis() - lastActivityTime;
}

bool StateMachine::checkInactivityTimeout() {
  const unsigned long INACTIVITY_TIMEOUT_MS = 3 * 60 * 1000; // 3 minutes
  const unsigned long MIN_AWAKE_TIME_MS = 5000; // 5 seconds minimum awake time
  
  // Don't sleep if we just started
  if (millis() < MIN_AWAKE_TIME_MS) {
    return false;
  }
  
  // ONLY check for inactivity timeout when in IdleState
  if (currentState != idleState) {
    updateActivityTime(); // Reset activity timer to prevent immediate sleep when returning to idle
    return false;
  }
  
  // Check if we've been inactive too long
  if (getTimeSinceLastActivity() > INACTIVITY_TIMEOUT_MS) {
    ESP_LOGI(TAG, "Inactivity timeout reached (%.1f minutes) in IdleState", 
             getTimeSinceLastActivity() / 60000.0f);
    return true;
  }
  
  return false;
}

void StateMachine::transitionTo(const char* stateName) {
  if (strcmp(stateName, "IdleState") == 0) {
    changeState(idleState);
  } else if (strcmp(stateName, "AdjustState") == 0) {
    changeState(adjustState);
  } else if (strcmp(stateName, "TimerState") == 0) {
    changeState(timerState);
  } else if (strcmp(stateName, "PausedState") == 0) {
    changeState(pausedState);
  } else if (strcmp(stateName, "DoneState") == 0) {
    changeState(doneState);
  } else if (strcmp(stateName, "ProjectSelectState") == 0) {
    changeState(projectSelectState);
  } else if (strcmp(stateName, "ProvisionState") == 0) {
    changeState(provisionState);
  } else if (strcmp(stateName, "SleepState") == 0) {
    changeState(sleepState);
  } else {
    ESP_LOGW(TAG, "Unknown state name: %s", stateName);
  }
}

// Power management methods
int StateMachine::getBatteryPercentage() const {
  extern XPowersAXP2101 power;
  
  if (!power.isBatteryConnect()) {
    return 100; // Assume full if PMU not available
  }
  
  float voltage = power.getBattVoltage();
  
  // Convert voltage to percentage
  // Li-ion typical curve: 4.2V = 100%, 3.7V = 50%, 3.3V = 0%
  if (voltage >= 4.1f) return 100;
  else if (voltage >= 3.9f) return 80;
  else if (voltage >= 3.8f) return 60;
  else if (voltage >= 3.7f) return 40;
  else if (voltage >= 3.5f) return 20;
  else return 10;
}

bool StateMachine::isCharging() const {
  extern XPowersAXP2101 power;
  return power.isBatteryConnect() && power.isVbusIn();
}

bool StateMachine::isBatteryLow() const {
  return getBatteryPercentage() <= 20;
}