#include "include/MemoryMonitor.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <string.h>

static const char* TAG = "MemoryMonitor";

// Static constants
const char* MemoryMonitor::NVS_NAMESPACE = "mem_monitor";
const char* MemoryMonitor::SNAPSHOT_KEY = "mem_snapshot";
const char* MemoryMonitor::TRANSITION_KEY = "transitions";

// Global instance
MemoryMonitor* g_memoryMonitor = nullptr;

MemoryMonitor::MemoryMonitor() :
  transitionIndex(0),
  nvsInitialized(false),
  watchdogTimeoutMs(10000),
  watchdogEnabled(false),
  baselineHeap(0),
  baselinePSRAM(0),
  leakCheckCounter(0)
{
  memset(&currentSnapshot, 0, sizeof(currentSnapshot));
  memset(transitionHistory, 0, sizeof(transitionHistory));
}

MemoryMonitor::~MemoryMonitor()
{
  cleanup();
}

bool MemoryMonitor::initialize()
{
  ESP_LOGI(TAG, "Initializing memory monitor...");
  
  initializeNVS();
  
  // Set memory baseline
  setMemoryBaseline();
  
  // Initialize current snapshot
  updateSnapshot("Initializing");
  
  ESP_LOGI(TAG, "Memory monitor initialized successfully");
  ESP_LOGI(TAG, "Baseline - Heap: %d bytes, PSRAM: %d bytes", 
           baselineHeap, baselinePSRAM);
  
  return true;
}

void MemoryMonitor::cleanup()
{
  if (watchdogEnabled) {
    disableWatchdog();
  }
  
  if (nvsInitialized) {
    nvs_close(nvsHandle);
    nvsInitialized = false;
  }
}

void MemoryMonitor::updateSnapshot(const char* currentStateName)
{
  if (!currentStateName) return;
  
  // Update memory information
  currentSnapshot.totalHeap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  currentSnapshot.freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  currentSnapshot.totalPSRAM = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  currentSnapshot.freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  currentSnapshot.maxAllocHeap = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  currentSnapshot.maxAllocPSRAM = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  currentSnapshot.minStackHighWaterMark = getCurrentStackHighWaterMark();
  currentSnapshot.timestamp = getCurrentTimestamp();
  currentSnapshot.stateTransitionCount = transitionIndex;
  
  strncpy(currentSnapshot.currentStateName, currentStateName, 
          sizeof(currentSnapshot.currentStateName) - 1);
  currentSnapshot.currentStateName[sizeof(currentSnapshot.currentStateName) - 1] = '\0';
  
  // Log memory status periodically
  if (leakCheckCounter % 10 == 0) {
    logMemoryStatus();
  }
  leakCheckCounter++;
}

MemorySnapshot MemoryMonitor::getCurrentSnapshot() const
{
  return currentSnapshot;
}

void MemoryMonitor::logMemoryStatus() const
{
  ESP_LOGI(TAG, "=== Memory Status [%s] ===", currentSnapshot.currentStateName);
  ESP_LOGI(TAG, "Heap: %d/%d bytes (%.1f%% used, largest: %d)", 
           currentSnapshot.totalHeap - currentSnapshot.freeHeap,
           currentSnapshot.totalHeap,
           getHeapUtilization(),
           currentSnapshot.maxAllocHeap);
  
  if (currentSnapshot.totalPSRAM > 0) {
    ESP_LOGI(TAG, "PSRAM: %d/%d bytes (%.1f%% used, largest: %d)", 
             currentSnapshot.totalPSRAM - currentSnapshot.freePSRAM,
             currentSnapshot.totalPSRAM,
             getPSRAMUtilization(),
             currentSnapshot.maxAllocPSRAM);
  }
  
  ESP_LOGI(TAG, "Stack high water mark: %d bytes", currentSnapshot.minStackHighWaterMark);
  ESP_LOGI(TAG, "State transitions: %d", currentSnapshot.stateTransitionCount);
}

void MemoryMonitor::detectMemoryLeaks()
{
  if (baselineHeap == 0) {
    setMemoryBaseline();
    return;
  }
  
  size_t currentHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t currentPSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  
  int32_t heapDelta = (int32_t)baselineHeap - (int32_t)currentHeap;
  int32_t psramDelta = (int32_t)baselinePSRAM - (int32_t)currentPSRAM;
  
  if (heapDelta > 1024) { // More than 1KB difference
    ESP_LOGW(TAG, "Potential heap memory leak detected: %d bytes lost", heapDelta);
    triggerEmergencySnapshot("HeapLeak");
  }
  
  if (psramDelta > 4096) { // More than 4KB difference
    ESP_LOGW(TAG, "Potential PSRAM memory leak detected: %d bytes lost", psramDelta);
    triggerEmergencySnapshot("PSRAMLeak");
  }
}

void MemoryMonitor::setMemoryBaseline()
{
  baselineHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  baselinePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  
  ESP_LOGI(TAG, "Memory baseline set - Heap: %d, PSRAM: %d", 
           baselineHeap, baselinePSRAM);
}

bool MemoryMonitor::saveSnapshotToNVS()
{
  if (!nvsInitialized) {
    ESP_LOGE(TAG, "NVS not initialized for snapshot save");
    return false;
  }
  
  esp_err_t err = nvs_set_blob(nvsHandle, SNAPSHOT_KEY, 
                               &currentSnapshot, sizeof(currentSnapshot));
  if (err != ESP_OK) {
    logError("saveSnapshotToNVS", err);
    return false;
  }
  
  err = nvs_set_blob(nvsHandle, TRANSITION_KEY, 
                     transitionHistory, sizeof(transitionHistory));
  if (err != ESP_OK) {
    logError("saveSnapshotToNVS (transitions)", err);
    return false;
  }
  
  err = nvs_commit(nvsHandle);
  if (err != ESP_OK) {
    logError("saveSnapshotToNVS (commit)", err);
    return false;
  }
  
  ESP_LOGI(TAG, "Memory snapshot saved to NVS");
  return true;
}

bool MemoryMonitor::loadSnapshotFromNVS(MemorySnapshot& snapshot)
{
  if (!nvsInitialized) {
    ESP_LOGE(TAG, "NVS not initialized for snapshot load");
    return false;
  }
  
  size_t required_size = sizeof(snapshot);
  esp_err_t err = nvs_get_blob(nvsHandle, SNAPSHOT_KEY, 
                               &snapshot, &required_size);
  
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "No recovery snapshot found in NVS");
    return false;
  } else if (err != ESP_OK) {
    logError("loadSnapshotFromNVS", err);
    return false;
  }
  
  ESP_LOGI(TAG, "Memory snapshot loaded from NVS");
  ESP_LOGI(TAG, "Recovered state: %s, timestamp: %llu", 
           snapshot.currentStateName, snapshot.timestamp);
  
  return true;
}

bool MemoryMonitor::hasRecoveryData() const
{
  if (!nvsInitialized) return false;
  
  size_t required_size;
  esp_err_t err = nvs_get_blob(nvsHandle, SNAPSHOT_KEY, NULL, &required_size);
  return (err == ESP_OK && required_size == sizeof(MemorySnapshot));
}

void MemoryMonitor::clearRecoveryData()
{
  if (!nvsInitialized) return;
  
  nvs_erase_key(nvsHandle, SNAPSHOT_KEY);
  nvs_erase_key(nvsHandle, TRANSITION_KEY);
  nvs_commit(nvsHandle);
  
  ESP_LOGI(TAG, "Recovery data cleared from NVS");
}

void MemoryMonitor::recordStateTransition(const char* fromState, const char* toState)
{
  if (!fromState || !toState) return;
  
  StateTransitionRecord& record = transitionHistory[transitionIndex % MAX_TRANSITION_RECORDS];
  
  strncpy(record.fromState, fromState, sizeof(record.fromState) - 1);
  record.fromState[sizeof(record.fromState) - 1] = '\0';
  
  strncpy(record.toState, toState, sizeof(record.toState) - 1);
  record.toState[sizeof(record.toState) - 1] = '\0';
  
  record.timestamp = getCurrentTimestamp();
  record.heapBefore = currentSnapshot.freeHeap;
  
  // Update snapshot for new state
  updateSnapshot(toState);
  
  record.heapAfter = currentSnapshot.freeHeap;
  record.heapDelta = (int32_t)record.heapAfter - (int32_t)record.heapBefore;
  
  transitionIndex++;
  
  ESP_LOGI(TAG, "State transition: %s -> %s (heap delta: %d bytes)", 
           fromState, toState, record.heapDelta);
  
  // Save snapshot after significant transitions
  if (transitionIndex % 5 == 0) {
    saveSnapshotToNVS();
  }
}

void MemoryMonitor::logTransitionHistory() const
{
  ESP_LOGI(TAG, "=== State Transition History ===");
  
  size_t count = (transitionIndex < MAX_TRANSITION_RECORDS) ? 
                 transitionIndex : MAX_TRANSITION_RECORDS;
  
  for (size_t i = 0; i < count; i++) {
    const StateTransitionRecord& record = transitionHistory[i];
    if (strlen(record.fromState) > 0) {
      ESP_LOGI(TAG, "[%d] %s -> %s (heap: %+d bytes)", 
               i, record.fromState, record.toState, record.heapDelta);
    }
  }
}

size_t MemoryMonitor::getTransitionCount() const
{
  return transitionIndex;
}

uint32_t MemoryMonitor::getCurrentStackHighWaterMark() const
{
  return uxTaskGetStackHighWaterMark(NULL);
}

bool MemoryMonitor::checkStackOverflow(uint32_t minBytes) const
{
  uint32_t highWaterMark = getCurrentStackHighWaterMark();
  if (highWaterMark < minBytes) {
    ESP_LOGW(TAG, "Stack overflow warning: only %d bytes remaining (min: %d)", 
             highWaterMark, minBytes);
    return true;
  }
  return false;
}

bool MemoryMonitor::enableWatchdog(uint32_t timeoutMs)
{
  if (watchdogEnabled) {
    ESP_LOGW(TAG, "Watchdog already enabled");
    return true;
  }
  
  esp_err_t err = esp_task_wdt_add(NULL);
  if (err != ESP_OK) {
    logError("enableWatchdog", err);
    return false;
  }
  
  watchdogEnabled = true;
  watchdogTimeoutMs = timeoutMs;
  
  ESP_LOGI(TAG, "Task watchdog enabled with %d ms timeout", timeoutMs);
  return true;
}

void MemoryMonitor::disableWatchdog()
{
  if (!watchdogEnabled) return;
  
  esp_task_wdt_delete(NULL);
  watchdogEnabled = false;
  
  ESP_LOGI(TAG, "Task watchdog disabled");
}

void MemoryMonitor::feedWatchdog()
{
  if (watchdogEnabled) {
    esp_task_wdt_reset();
  }
}

bool MemoryMonitor::isWatchdogEnabled() const
{
  return watchdogEnabled;
}

void MemoryMonitor::logDetailedMemoryInfo() const
{
  ESP_LOGI(TAG, "=== Detailed Memory Information ===");
  
  // Heap analysis
  multi_heap_info_t heapInfo;
  heap_caps_get_info(&heapInfo, MALLOC_CAP_8BIT);
  ESP_LOGI(TAG, "Heap - Total: %d, Free: %d, Allocated: %d, Largest: %d", 
           heapInfo.total_free_bytes + heapInfo.total_allocated_bytes,
           heapInfo.total_free_bytes, heapInfo.total_allocated_bytes,
           heapInfo.largest_free_block);
  
  // PSRAM analysis
  if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0) {
    multi_heap_info_t psramInfo;
    heap_caps_get_info(&psramInfo, MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "PSRAM - Total: %d, Free: %d, Allocated: %d, Largest: %d", 
             psramInfo.total_free_bytes + psramInfo.total_allocated_bytes,
             psramInfo.total_free_bytes, psramInfo.total_allocated_bytes,
             psramInfo.largest_free_block);
  }
  
  ESP_LOGI(TAG, "Stack high water mark: %d bytes", getCurrentStackHighWaterMark());
}

bool MemoryMonitor::isMemoryFragmented() const
{
  size_t totalFree = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  
  // Consider fragmented if largest block is less than 50% of total free
  return (largestBlock < totalFree / 2);
}

float MemoryMonitor::getHeapUtilization() const
{
  if (currentSnapshot.totalHeap == 0) return 0.0f;
  return ((float)(currentSnapshot.totalHeap - currentSnapshot.freeHeap) / 
          currentSnapshot.totalHeap) * 100.0f;
}

float MemoryMonitor::getPSRAMUtilization() const
{
  if (currentSnapshot.totalPSRAM == 0) return 0.0f;
  return ((float)(currentSnapshot.totalPSRAM - currentSnapshot.freePSRAM) / 
          currentSnapshot.totalPSRAM) * 100.0f;
}

void MemoryMonitor::triggerEmergencySnapshot(const char* reason)
{
  ESP_LOGW(TAG, "Emergency snapshot triggered: %s", reason ? reason : "Unknown");
  
  updateSnapshot("Emergency");
  saveSnapshotToNVS();
  logDetailedMemoryInfo();
}

bool MemoryMonitor::performEmergencyRecovery()
{
  ESP_LOGI(TAG, "Performing emergency recovery...");
  
  if (!hasRecoveryData()) {
    ESP_LOGI(TAG, "No recovery data available");
    return false;
  }
  
  MemorySnapshot recoverySnapshot;
  if (!loadSnapshotFromNVS(recoverySnapshot)) {
    ESP_LOGE(TAG, "Failed to load recovery snapshot");
    return false;
  }
  
  ESP_LOGI(TAG, "Recovery data found from state: %s", recoverySnapshot.currentStateName);
  ESP_LOGI(TAG, "Recovery timestamp: %llu", recoverySnapshot.timestamp);
  
  // Clear recovery data after successful load
  clearRecoveryData();
  
  return true;
}

void MemoryMonitor::dumpMemoryLayout() const
{
  ESP_LOGI(TAG, "=== Memory Layout Dump ===");
  
  // Print all heap capabilities
  ESP_LOGI(TAG, "Internal RAM (8-bit): %d bytes", 
           heap_caps_get_total_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
  ESP_LOGI(TAG, "Internal RAM (32-bit): %d bytes", 
           heap_caps_get_total_size(MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL));
  ESP_LOGI(TAG, "SPIRAM (PSRAM): %d bytes", 
           heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
  ESP_LOGI(TAG, "DMA capable: %d bytes", 
           heap_caps_get_total_size(MALLOC_CAP_DMA));
}

void MemoryMonitor::validateHeapIntegrity() const
{
  if (!heap_caps_check_integrity_all(true)) {
    ESP_LOGE(TAG, "CRITICAL: Heap integrity check failed! Heap corruption detected!");
  } else {
    ESP_LOGI(TAG, "Heap integrity check passed");
  }
}

void MemoryMonitor::initializeNVS()
{
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandle);
  if (err != ESP_OK) {
    logError("initializeNVS", err);
    return;
  }
  
  nvsInitialized = true;
  ESP_LOGI(TAG, "NVS initialized successfully");
}

void MemoryMonitor::logError(const char* function, esp_err_t error) const
{
  ESP_LOGE(TAG, "%s failed: %s", function, esp_err_to_name(error));
}

uint64_t MemoryMonitor::getCurrentTimestamp() const
{
  return esp_timer_get_time();
}

void MemoryMonitor::shiftTransitionHistory()
{
  // History is managed as a circular buffer, no shifting needed
}