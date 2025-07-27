#include "include/State.h"

void State::dumpState() const
{
  ESP_LOGI(getLogTag(), "=== STATE DUMP: %s ===", getStateName());
  ESP_LOGI(getLogTag(), "Entry count: %lu", stats.entryCount);
  ESP_LOGI(getLogTag(), "Update count: %lu", stats.updateCount);
  ESP_LOGI(getLogTag(), "Total time in state: %llu ms", stats.totalTimeInState / 1000);
  ESP_LOGI(getLogTag(), "Current time in state: %lu ms", getTimeInState());
  ESP_LOGI(getLogTag(), "Max update time: %llu us", stats.maxUpdateTime);
  ESP_LOGI(getLogTag(), "Min update time: %llu us", stats.minUpdateTime);
  ESP_LOGI(getLogTag(), "Peak memory usage: %u bytes", stats.peakMemoryUsage);
  ESP_LOGI(getLogTag(), "Min stack space: %u words", stats.minStackSpace);
  
  logMemoryAndStack("DUMP");
}

void State::logMemoryAndStack(const char* context) const
{
  // Current memory status
  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  size_t used_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT) - free_heap;
  size_t used_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM) - free_psram;
  
  // Stack monitoring
  UBaseType_t stackSpace = uxTaskGetStackHighWaterMark(NULL);
  
  ESP_LOGD(getLogTag(), "[%s] Memory - Heap: %u/%u, PSRAM: %u/%u, Stack: %u words", 
           context, used_heap, free_heap, used_psram, free_psram, stackSpace);
  
  // Update peak memory tracking (non-const hack for statistics)
  State* mutableThis = const_cast<State*>(this);
  if (used_heap > mutableThis->stats.peakMemoryUsage) {
    mutableThis->stats.peakMemoryUsage = used_heap;
  }
  
  // Update minimum stack space tracking
  if (mutableThis->stats.minStackSpace == 0 || stackSpace < mutableThis->stats.minStackSpace) {
    mutableThis->stats.minStackSpace = stackSpace;
  }
  
  // Warning for low resources
  if (free_heap < 10240) { // Less than 10KB
    ESP_LOGW(getLogTag(), "[%s] Low heap memory: %u bytes remaining", context, free_heap);
  }
  
  if (stackSpace < 256) { // Less than 256 words (1KB)
    ESP_LOGW(getLogTag(), "[%s] Low stack space: %u words remaining", context, stackSpace);
  }
}

void State::logStateStatistics() const
{
  if (stats.entryCount == 0) {
    return; // No statistics to log
  }
  
  ESP_LOGI(getLogTag(), "=== STATISTICS: %s ===", getStateName());
  ESP_LOGI(getLogTag(), "Entries: %lu, Updates: %lu", stats.entryCount, stats.updateCount);
  
  if (stats.entryCount > 0) {
    uint64_t avgTimePerEntry = stats.totalTimeInState / stats.entryCount;
    ESP_LOGI(getLogTag(), "Avg time per entry: %llu ms", avgTimePerEntry / 1000);
  }
  
  if (stats.updateCount > 0) {
    uint64_t avgUpdateTime = (stats.maxUpdateTime + stats.minUpdateTime) / 2;
    ESP_LOGI(getLogTag(), "Update times - Min: %llu us, Max: %llu us, Avg: %llu us", 
             stats.minUpdateTime, stats.maxUpdateTime, avgUpdateTime);
  }
  
  ESP_LOGI(getLogTag(), "Peak memory: %u bytes, Min stack: %u words", 
           stats.peakMemoryUsage, stats.minStackSpace);
}