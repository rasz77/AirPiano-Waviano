#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

// ============================================
// TIME-SLICED TASK SCHEDULER
// ============================================

// Forward declarations for task functions
void processIRTask();
void processSerialTask();
void processUltrasonicTask();
void processAudioTask();
void processPlaybackTask();

// Scheduler configuration
#define SCHEDULER_TICK_MS 10        // Main loop target cycle time: 10ms (100Hz)
#define MAX_TASKS 8                 // Maximum number of scheduled tasks

// Task structure for scheduled execution
struct ScheduledTask {
  const char* name;                 // Task name for debugging
  unsigned long last_run_time;      // Last execution timestamp
  unsigned long interval_ms;        // Execution interval in milliseconds
  void (*task_function)();          // Pointer to task function
  bool enabled;                     // Task enable flag
  unsigned long execution_count;    // Debug: execution counter
  unsigned long max_duration_ms;    // Debug: maximum execution time
};

// Task scheduler state
struct TaskScheduler {
  ScheduledTask tasks[MAX_TASKS];
  int task_count;
  unsigned long loop_count;
  unsigned long max_loop_duration;
  unsigned long last_loop_start;
};

// Global scheduler instance
TaskScheduler scheduler = {{}, 0, 0, 0, 0};

/**
 * Register a task with the scheduler
 * @param name Task name for debugging
 * @param task_func Function pointer to task
 * @param interval_ms Execution interval in milliseconds
 * @param enabled Initial enabled state
 * @return Task ID or -1 if scheduler full
 */
int registerTask(const char* name, void (*task_func)(), unsigned long interval_ms, bool enabled = true) {
  if (scheduler.task_count >= MAX_TASKS) {
    return -1;  // Scheduler full
  }

  int task_id = scheduler.task_count++;
  ScheduledTask* task = &scheduler.tasks[task_id];

  task->name = name;
  task->task_function = task_func;
  task->interval_ms = interval_ms;
  task->enabled = enabled;
  task->last_run_time = 0;
  task->execution_count = 0;
  task->max_duration_ms = 0;

  return task_id;
}

/**
 * Enable or disable a task
 * @param task_id Task ID returned from registerTask
 * @param enabled true to enable, false to disable
 */
void setTaskEnabled(int task_id, bool enabled) {
  if (task_id >= 0 && task_id < scheduler.task_count) {
    scheduler.tasks[task_id].enabled = enabled;
  }
}

/**
 * Run the task scheduler (call from main loop)
 * Executes tasks whose intervals have elapsed
 */
void runScheduler() {
  unsigned long loop_start = millis();
  unsigned long now = loop_start;

  // Track loop timing
  if (scheduler.last_loop_start > 0) {
    unsigned long loop_duration = loop_start - scheduler.last_loop_start;
    if (loop_duration > scheduler.max_loop_duration) {
      scheduler.max_loop_duration = loop_duration;
    }
  }
  scheduler.last_loop_start = loop_start;
  scheduler.loop_count++;

  // Execute scheduled tasks
  for (int i = 0; i < scheduler.task_count; i++) {
    ScheduledTask* task = &scheduler.tasks[i];

    if (!task->enabled) {
      continue;  // Skip disabled tasks
    }

    // Check if task interval has elapsed
    if (now - task->last_run_time >= task->interval_ms) {
      unsigned long task_start = millis();

      // Execute the task
      task->task_function();

      // Update task statistics
      unsigned long task_duration = millis() - task_start;
      if (task_duration > task->max_duration_ms) {
        task->max_duration_ms = task_duration;
      }

      task->last_run_time = now;
      task->execution_count++;
    }
  }
}

/**
 * Print scheduler statistics (for debugging)
 */
void printSchedulerStats() {
  Serial.println(F("\n=== SCHEDULER STATISTICS ==="));
  Serial.print(F("Loop count: "));
  Serial.println(scheduler.loop_count);
  Serial.print(F("Max loop duration: "));
  Serial.print(scheduler.max_loop_duration);
  Serial.println(F("ms"));

  Serial.println(F("\nTASK STATISTICS:"));
  for (int i = 0; i < scheduler.task_count; i++) {
    ScheduledTask* task = &scheduler.tasks[i];
    Serial.print(F("  "));
    Serial.print(task->name);
    Serial.print(F(": "));
    Serial.print(task->execution_count);
    Serial.print(F(" runs, max "));
    Serial.print(task->max_duration_ms);
    Serial.print(F("ms, interval "));
    Serial.print(task->interval_ms);
    Serial.println(F("ms"));
  }
  Serial.println(F("============================\n"));
}

/**
 * Reset scheduler statistics
 */
void resetSchedulerStats() {
  scheduler.loop_count = 0;
  scheduler.max_loop_duration = 0;

  for (int i = 0; i < scheduler.task_count; i++) {
    scheduler.tasks[i].execution_count = 0;
    scheduler.tasks[i].max_duration_ms = 0;
  }
}

// ============================================
// TIMING UTILITIES
// ============================================

/**
 * Maintain consistent loop timing
 * @param target_ms Target loop duration in milliseconds
 */
void maintainLoopTiming(unsigned long target_ms) {
  static unsigned long last_warning = 0;
  unsigned long loop_duration = millis() - scheduler.last_loop_start;

  if (loop_duration < target_ms) {
    // Loop finished early, delay to maintain timing
    delay(target_ms - loop_duration);
  } else if (loop_duration > target_ms + 5) {
    // Loop took too long, warn occasionally
    if (millis() - last_warning > 5000) {
      Serial.print(F("[SCHEDULER] Loop overrun: "));
      Serial.print(loop_duration);
      Serial.print(F("ms (target: "));
      Serial.print(target_ms);
      Serial.println(F("ms)"));
      last_warning = millis();
    }
  }
}

#endif // SCHEDULER_H
