#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include "config.h"

// ============================================
// FORWARD DECLARATIONS
// ============================================
// These are defined in ir_remote.h but we need them here
// to avoid circular dependency issues

#ifndef IR_COMMAND_QUEUE_SIZE
  #define IR_COMMAND_QUEUE_SIZE 4  // Must match definition in ir_remote.h
#endif

// Forward declare IRStats structure
struct IRStats {
  unsigned long total_decoded;
  unsigned long total_queued;
  unsigned long total_processed;
  unsigned long queue_overflows;
  unsigned long invalid_commands;
  unsigned long last_decode_time;
};

// ============================================
// DEBUG SYSTEM - SIMPLIFIED TWO-MODE
// ============================================
// Production Mode: No debug output, zero overhead
// Verbose Mode: Complete diagnostics with timestamps

// Debug mode flag (set in config.h or here)
#ifndef DEBUG_VERBOSE
  #define DEBUG_VERBOSE false  // Default to production mode
#endif

// Global debug state (can be toggled at runtime)
#if DEBUG_VERBOSE
bool debug_enabled = true;
#else
bool debug_enabled = false;
#endif

// ============================================
// DEBUG DATA STRUCTURES (Only in verbose mode)
// ============================================

#if DEBUG_VERBOSE

// ============================================
// TIMING STATISTICS
// ============================================

struct LoopTimingStats {
  unsigned long iterations = 0;
  unsigned long min_duration = 999999;
  unsigned long max_duration = 0;
  unsigned long total_duration = 0;
  unsigned long last_10_durations[10] = {0};
  int last_10_index = 0;
  unsigned long overrun_count = 0;  // Loops that exceeded target
  unsigned long last_overrun_time = 0;
};

LoopTimingStats loop_stats;

// ============================================
// IR STATISTICS (Extended)
// ============================================

struct IRDebugStats {
  unsigned long last_decode_time = 0;
  unsigned long decode_count = 0;
  unsigned long queue_high_water = 0;  // Max queue depth seen
  unsigned long last_command = 0;
  unsigned long last_flags = 0;
  unsigned long zero_commands = 0;  // Count of 0x0 commands
};

IRDebugStats ir_debug_stats;

// ============================================
// AUDIO STATE TRACKING
// ============================================

struct AudioDebugInfo {
  unsigned long state_changes = 0;
  unsigned long last_state_change = 0;
  int last_note_index = -1;
  unsigned long last_note_duration = 0;
  int last_volume_percent = 0;
  const char* current_state_name = "IDLE";
};

AudioDebugInfo audio_debug;

#endif // DEBUG_VERBOSE

// ============================================
// MODE TRACKING
// ============================================

const char* mode_names[] = {
  "MENU",
  "FREE_PLAY",
  "RECORDING",
  "PLAYBACK"
};

SystemMode last_mode = MODE_MENU;
unsigned long last_mode_change = 0;

// ============================================
// DEBUG OUTPUT FUNCTIONS
// ============================================

#if DEBUG_VERBOSE

/**
 * Print timestamp in [seconds.ms] format
 */
void printTimestamp() {
  if (!debug_enabled) return;

  unsigned long now = millis();
  unsigned long seconds = now / 1000;
  unsigned long ms = now % 1000;

  Serial.print(F("["));
  if (seconds < 10) Serial.print(F("0"));
  if (seconds < 100) Serial.print(F("0"));
  if (seconds < 1000) Serial.print(F("0"));
  Serial.print(seconds);
  Serial.print(F("."));
  if (ms < 10) Serial.print(F("00"));
  else if (ms < 100) Serial.print(F("0"));
  Serial.print(ms);
  Serial.print(F("] "));
}

/**
 * Debug print with subsystem prefix
 */
void debugPrint(const char* subsystem, const char* message) {
  if (!debug_enabled) return;

  printTimestamp();
  Serial.print(F("["));
  Serial.print(subsystem);
  Serial.print(F("] "));
  Serial.println(message);
}

/**
 * Debug print with value
 */
void debugPrintValue(const char* subsystem, const char* message, long value) {
  if (!debug_enabled) return;

  printTimestamp();
  Serial.print(F("["));
  Serial.print(subsystem);
  Serial.print(F("] "));
  Serial.print(message);
  Serial.print(F(": "));
  Serial.println(value);
}

/**
 * Debug print with hex value
 */
void debugPrintHex(const char* subsystem, const char* message, unsigned long value) {
  if (!debug_enabled) return;

  printTimestamp();
  Serial.print(F("["));
  Serial.print(subsystem);
  Serial.print(F("] "));
  Serial.print(message);
  Serial.print(F(": 0x"));
  Serial.println(value, HEX);
}

// ============================================
// IR DEBUG FUNCTIONS
// ============================================

/**
 * Log IR decode event
 */
void debugIRDecode(uint16_t command, uint8_t flags, uint8_t protocol) {
  if (!debug_enabled) return;

  ir_debug_stats.decode_count++;
  ir_debug_stats.last_decode_time = millis();
  ir_debug_stats.last_command = command;
  ir_debug_stats.last_flags = flags;

  if (command == 0) {
    ir_debug_stats.zero_commands++;
  }

  printTimestamp();
  Serial.print(F("[IR] Decoded: cmd=0x"));
  Serial.print(command, HEX);
  Serial.print(F(" flags=0x"));
  Serial.print(flags, HEX);
  Serial.print(F(" proto="));
  Serial.println(protocol);
}

/**
 * Extended IR decode for troubleshooting
 */
void debugIRDecodeExtended(uint32_t rawData, uint16_t address, uint16_t command, uint8_t flags) {
  if (!debug_enabled) return;

  printTimestamp();
  Serial.print(F("[IR-EXT] Raw=0x"));
  Serial.print(rawData, HEX);
  Serial.print(F(" Addr=0x"));
  Serial.print(address, HEX);
  Serial.print(F(" Cmd=0x"));
  Serial.print(command, HEX);
  Serial.print(F(" Flags=0x"));
  Serial.println(flags, HEX);
}

/**
 * Log IR queue operation
 */
void debugIRQueue(const char* operation, int head, int tail, int depth, int slot = -1) {
  if (!debug_enabled) return;

  if (depth > ir_debug_stats.queue_high_water) {
    ir_debug_stats.queue_high_water = depth;
  }

  printTimestamp();
  Serial.print(F("[QUEUE] "));
  Serial.print(operation);
  if (slot >= 0) {
    Serial.print(F(" @slot["));
    Serial.print(slot);
    Serial.print(F("]"));
  }
  Serial.print(F(" (head="));
  Serial.print(head);
  Serial.print(F(" tail="));
  Serial.print(tail);
  Serial.print(F(" depth="));
  Serial.print(depth);
  Serial.println(F("/8)"));
}

/**
 * Log IR command execution
 */
void debugIRExecute(uint16_t command, const char* cmd_string) {
  if (!debug_enabled) return;

  printTimestamp();
  Serial.print(F("[IR] Execute: 0x"));
  Serial.print(command, HEX);
  Serial.print(F(" -> \""));
  Serial.print(cmd_string);
  Serial.println(F("\""));
}

// ============================================
// AUDIO DEBUG FUNCTIONS
// ============================================

/**
 * Log audio state change
 */
void debugAudioState(const char* old_state, const char* new_state) {
  if (!debug_enabled) return;

  audio_debug.state_changes++;
  audio_debug.last_state_change = millis();
  audio_debug.current_state_name = new_state;

  printTimestamp();
  Serial.print(F("[AUDIO] State: "));
  Serial.print(old_state);
  Serial.print(F(" -> "));
  Serial.println(new_state);
}

/**
 * Log audio note request
 */
void debugAudioRequest(int note_index, unsigned long duration, int volume) {
  if (!debug_enabled) return;

  audio_debug.last_note_index = note_index;
  audio_debug.last_note_duration = duration;
  audio_debug.last_volume_percent = volume;

  printTimestamp();
  Serial.print(F("[AUDIO] Request: note="));
  Serial.print(note_index);
  Serial.print(F(" dur="));
  Serial.print(duration);
  Serial.print(F("ms vol="));
  Serial.print(volume);
  Serial.println(F("%"));
}

/**
 * Log audio pulse parameters
 */
void debugAudioPulse(int on_ms, int off_ms) {
  if (!debug_enabled) return;

  printTimestamp();
  Serial.print(F("[AUDIO] Pulse: ON="));
  Serial.print(on_ms);
  Serial.print(F("ms OFF="));
  Serial.print(off_ms);
  Serial.println(F("ms"));
}

// ============================================
// MODE DEBUG FUNCTIONS
// ============================================

/**
 * Log system mode change
 */
void debugModeChange(SystemMode old_mode, SystemMode new_mode) {
  if (!debug_enabled) return;

  last_mode = new_mode;
  last_mode_change = millis();

  printTimestamp();
  Serial.print(F("[MODE] "));
  Serial.print(mode_names[old_mode]);
  Serial.print(F(" -> "));
  Serial.println(mode_names[new_mode]);
}

// ============================================
// LOOP TIMING FUNCTIONS
// ============================================

/**
 * Update loop timing statistics
 */
void updateLoopStats(unsigned long duration) {
  loop_stats.iterations++;
  loop_stats.total_duration += duration;

  if (duration < loop_stats.min_duration) {
    loop_stats.min_duration = duration;
  }

  if (duration > loop_stats.max_duration) {
    loop_stats.max_duration = duration;
  }

  // Track last 10 durations
  loop_stats.last_10_durations[loop_stats.last_10_index] = duration;
  loop_stats.last_10_index = (loop_stats.last_10_index + 1) % 10;

  // Count overruns
  if (duration > MAX_LOOP_DURATION_MS) {
    loop_stats.overrun_count++;
    loop_stats.last_overrun_time = millis();

    if (debug_enabled) {
      printTimestamp();
      Serial.print(F("[TIMING] OVERRUN: "));
      Serial.print(duration);
      Serial.println(F("ms"));
    }
  }
}

// ============================================
// STATUS DISPLAY FUNCTIONS
// ============================================

/**
 * Print comprehensive system status
 */
void printSystemStatus() {
  Serial.println(F("\n========================================"));
  Serial.println(F("         SYSTEM STATUS"));
  Serial.println(F("========================================"));

  // System info
  Serial.print(F("Uptime: "));
  Serial.print(millis() / 1000.0, 1);
  Serial.println(F("s"));

  Serial.print(F("Debug: "));
  Serial.println(debug_enabled ? F("VERBOSE") : F("OFF"));

  Serial.print(F("Mode: "));
  Serial.println(mode_names[current_mode]);

  // IR Queue status
  extern int ir_queue_head, ir_queue_tail;
  int queue_depth = (ir_queue_head - ir_queue_tail + IR_COMMAND_QUEUE_SIZE) % IR_COMMAND_QUEUE_SIZE;

  Serial.print(F("IR Queue: "));
  Serial.print(queue_depth);
  Serial.print(F("/"));
  Serial.print(IR_COMMAND_QUEUE_SIZE);
  Serial.println(F(" used"));

  // Audio state
  Serial.print(F("Audio: "));
  Serial.println(audio_debug.current_state_name);

  // Loop timing
  unsigned long avg_duration = 0;
  if (loop_stats.iterations > 0) {
    avg_duration = loop_stats.total_duration / loop_stats.iterations;
  }

  Serial.print(F("Loop: current="));
  Serial.print(loop_stats.last_10_durations[(loop_stats.last_10_index + 9) % 10]);
  Serial.print(F("ms avg="));
  Serial.print(avg_duration);
  Serial.print(F("ms max="));
  Serial.print(loop_stats.max_duration);
  Serial.println(F("ms"));

  Serial.println(F("========================================\n"));
}

/**
 * Print detailed statistics
 */
void printDetailedStats() {
  Serial.println(F("\n========================================"));
  Serial.println(F("       DETAILED STATISTICS"));
  Serial.println(F("========================================"));

  // IR Statistics
  Serial.println(F("\n--- IR STATS ---"));
  Serial.print(F("Total decoded: "));
  Serial.println(ir_debug_stats.decode_count);

  Serial.print(F("Zero commands: "));
  Serial.println(ir_debug_stats.zero_commands);

  Serial.print(F("Queue high water: "));
  Serial.print(ir_debug_stats.queue_high_water);
  Serial.println(F("/8"));

  Serial.print(F("Last command: 0x"));
  Serial.println(ir_debug_stats.last_command, HEX);

  Serial.print(F("Last decode: "));
  Serial.print((millis() - ir_debug_stats.last_decode_time) / 1000.0, 1);
  Serial.println(F("s ago"));

  // Also print IR library stats if available
  extern IRStats ir_stats;
  Serial.print(F("Queued: "));
  Serial.println(ir_stats.total_queued);
  Serial.print(F("Processed: "));
  Serial.println(ir_stats.total_processed);
  Serial.print(F("Overflows: "));
  Serial.println(ir_stats.queue_overflows);

  // Audio Statistics
  Serial.println(F("\n--- AUDIO STATS ---"));
  Serial.print(F("State changes: "));
  Serial.println(audio_debug.state_changes);

  if (audio_debug.last_note_index >= 0) {
    Serial.print(F("Last note: "));
    Serial.print(audio_debug.last_note_index);
    Serial.print(F(" ("));
    Serial.print(audio_debug.last_note_duration);
    Serial.print(F("ms @ "));
    Serial.print(audio_debug.last_volume_percent);
    Serial.println(F("%)"));
  }

  // Loop Timing Statistics
  Serial.println(F("\n--- TIMING STATS ---"));
  Serial.print(F("Iterations: "));
  Serial.println(loop_stats.iterations);

  Serial.print(F("Min/Avg/Max: "));
  Serial.print(loop_stats.min_duration);
  Serial.print(F("/"));
  if (loop_stats.iterations > 0) {
    Serial.print(loop_stats.total_duration / loop_stats.iterations);
  } else {
    Serial.print(F("0"));
  }
  Serial.print(F("/"));
  Serial.print(loop_stats.max_duration);
  Serial.println(F("ms"));

  Serial.print(F("Overruns: "));
  Serial.print(loop_stats.overrun_count);
  if (loop_stats.overrun_count > 0) {
    Serial.print(F(" (last: "));
    Serial.print((millis() - loop_stats.last_overrun_time) / 1000.0, 1);
    Serial.print(F("s ago)"));
  }
  Serial.println();

  Serial.print(F("Last 10 loops: "));
  for (int i = 0; i < 10; i++) {
    int idx = (loop_stats.last_10_index + i) % 10;
    Serial.print(loop_stats.last_10_durations[idx]);
    if (i < 9) Serial.print(F(", "));
  }
  Serial.println(F("ms"));

  Serial.println(F("========================================\n"));
}

/**
 * Toggle debug mode
 */
void toggleDebugMode() {
  debug_enabled = !debug_enabled;

  Serial.print(F("\n*** DEBUG MODE: "));
  Serial.print(debug_enabled ? F("VERBOSE") : F("OFF"));
  Serial.println(F(" ***\n"));

  if (debug_enabled) {
    Serial.println(F("Verbose debugging enabled. Commands:"));
    Serial.println(F("  DBG    - Toggle debug mode"));
    Serial.println(F("  STATUS - Show system status"));
    Serial.println(F("  STATS  - Show detailed statistics"));
    Serial.println(F("  RESET  - Reset statistics\n"));
  }
}

/**
 * Reset all statistics
 */
void resetDebugStats() {
  // Reset loop stats
  loop_stats = LoopTimingStats();

  // Reset IR stats
  ir_debug_stats = IRDebugStats();
  extern IRStats ir_stats;
  ir_stats = {0, 0, 0, 0, 0, 0};

  // Reset audio stats
  audio_debug = AudioDebugInfo();
  audio_debug.current_state_name = "IDLE";

  Serial.println(F("\n*** All statistics reset ***\n"));
}

// ============================================
// INITIALIZATION
// ============================================

/**
 * Initialize debug system
 */
void initDebugSystem() {
  if (DEBUG_VERBOSE) {
    debug_enabled = true;
    Serial.println(F("\n*** DEBUG MODE: VERBOSE ***"));
    Serial.println(F("Type 'DBG' to toggle, 'STATUS' for status\n"));
  } else {
    debug_enabled = false;
  }
}

#else // !DEBUG_VERBOSE

// ============================================
// STUB FUNCTIONS FOR PRODUCTION MODE
// ============================================
// These do nothing but allow code to compile

inline void printTimestamp() {}
inline void debugPrint(const char*, const char*) {}
inline void debugPrintValue(const char*, const char*, long) {}
inline void debugPrintHex(const char*, const char*, unsigned long) {}
inline void debugIRDecode(uint16_t, uint8_t, uint8_t) {}
inline void debugIRDecodeExtended(uint32_t, uint16_t, uint16_t, uint8_t) {}
inline void debugIRQueue(const char*, int, int, int, int = -1) {}
inline void debugIRExecute(uint16_t, const char*) {}
inline void debugAudioState(const char*, const char*) {}
inline void debugAudioRequest(int, unsigned long, int) {}
inline void debugAudioPulse(int, int) {}
inline void debugModeChange(SystemMode, SystemMode) {}
inline void updateLoopStats(unsigned long) {}
inline void printSystemStatus() {}
inline void printDetailedStats() {}
inline void toggleDebugMode() {}
inline void resetDebugStats() {}
inline void initDebugSystem() {}

#endif // DEBUG_VERBOSE

#endif // DEBUG_H
