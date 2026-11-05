#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include "config.h"
#include "note_mapping.h"
#include "recording.h"
#include "playback.h"
#include "debug.h"  // Include debug system

// Forward declarations removed - IRCAL mode eliminated

// ============================================
// UI STATE
// ============================================

// Current overlap strategy for playback
OverlapStrategy current_overlap_strategy = DEFAULT_OVERLAP_STRATEGY;

// Input buffer for reading complete lines
// REDUCED from 32 to 16 to save RAM (saves 16 bytes)
#define INPUT_BUFFER_SIZE 16
char input_buffer[INPUT_BUFFER_SIZE];
int buffer_index = 0;

// ============================================
// MENU DISPLAY FUNCTIONS
// ============================================

/**
 * Print the main menu
 */
void printMainMenu() {
  Serial.println(F("\n========================================"));
  Serial.println(F("        PIANO AIR - Main Menu"));
  Serial.println(F("========================================"));
  Serial.println(F("\nCONTROL OPTIONS:"));
  Serial.println(F("  - Serial Monitor (type commands below)"));
  Serial.println(F("  - IR Remote (see button reference)"));
  Serial.println(F("\nFREE PLAY & RECORDING:"));
  Serial.println(F("  0 - Toggle free play mode (IR: 0 button)"));
  Serial.println(F("  R[1-3] - Record to slot (IR: 5-7)"));
  Serial.println(F("  S - Stop recording (IR: Down)"));
  Serial.println(F("\nPLAYBACK:"));
  Serial.println(F("  P[1-3] - Play slot (IR: 1-3)"));
  Serial.println(F("  PA - Play all slots (IR: Play/Pause)"));
  Serial.println(F("  X - Stop playback (IR: Power)"));
  Serial.println(F("\nMANAGEMENT:"));
  Serial.println(F("  L - List recordings (IR: 9)"));
  Serial.println(F("  C[1-3] - Clear slot (IR: EQ after play)"));
  Serial.println(F("  CA - Clear all (IR: ST/REPT)"));
  Serial.println(F("  M[1-4] - Overlap mode (IR: <</>>)"));
  Serial.println(F("\nOVERLAP MODES:"));
  Serial.println(F("  M1 - Priority High"));
  Serial.println(F("  M2 - Priority Low"));
  Serial.println(F("  M3 - Alternate"));
  Serial.println(F("  M4 - Drop"));
  Serial.println(F("========================================\n"));
}

/**
 * Print current system status
 */
void printStatus() {
  Serial.print(F("Mode: "));

  if (isRecording()) {
    Serial.print(F("RECORDING to Slot "));
    Serial.print(getActiveRecordingSlot() + 1);
    int note_count = getSlotNoteCount(getActiveRecordingSlot());
    Serial.print(F(" ["));
    Serial.print(note_count);
    Serial.print(F("/"));
    Serial.print(MAX_NOTES_PER_SLOT);
    Serial.println(F(" notes]"));
  } else if (isPlaying()) {
    Serial.print(F("PLAYING"));
    int current, total;
    if (getPlaybackProgress(&current, &total)) {
      Serial.print(F(" ["));
      Serial.print(current);
      Serial.print(F("/"));
      Serial.print(total);
      Serial.print(F(" events]"));
    }
    Serial.println();
  } else {
    Serial.println(F("FREE PLAY"));
  }
}

/**
 * List all recordings
 */
void listRecordings() {
  Serial.println(F("\n--- Recording Slots ---"));

  bool has_recordings = false;

  for (int i = 0; i < NUM_RECORDING_SLOTS; i++) {
    Serial.print(F("Slot "));
    Serial.print(i + 1);
    Serial.print(F(": "));

    if (isSlotActive(i)) {
      has_recordings = true;
      int note_count = getSlotNoteCount(i);
      unsigned long duration_ms = getRecordingDuration(i);

      Serial.print(note_count);
      Serial.print(F(" notes, "));
      Serial.print(duration_ms / 1000.0, 1);
      Serial.println(F("s"));
    } else {
      Serial.println(F("[Empty]"));
    }
  }

  if (!has_recordings) {
    Serial.println(F("\nNo recordings found."));
  }

  Serial.println(F("----------------------\n"));
}

/**
 * Print overlap strategy name
 */
void printOverlapStrategy(OverlapStrategy strategy) {
  switch (strategy) {
    case OVERLAP_PRIORITY_HIGH:
      Serial.print(F("Priority High"));
      break;
    case OVERLAP_PRIORITY_LOW:
      Serial.print(F("Priority Low"));
      break;
    case OVERLAP_ALTERNATE:
      Serial.print(F("Alternate"));
      break;
    case OVERLAP_DROP:
      Serial.print(F("Drop"));
      break;
    default:
      Serial.print(F("Unknown"));
  }
}

// ============================================
// COMMAND PARSING FUNCTIONS
// ============================================

/**
 * Parse and handle serial input commands
 * @param cmd Input command string
 * @return System mode after command execution
 */
SystemMode handleSerialCommand(char* cmd) {
  // Skip empty commands
  if (cmd[0] == '\0') {
    return current_mode;
  }

  // Convert first character to uppercase for easier parsing
  char input = cmd[0];
  if (input >= 'a' && input <= 'z') {
    input = input - 32;
  }

  // ---- FREE PLAY MODE (TOGGLE) ----
  if (input == '0') {
    if (current_mode != MODE_FREE_PLAY) {
      Serial.println(F("\nFree play mode activated!"));
      return MODE_FREE_PLAY;
    } else {
      Serial.println(F("\nFree play mode deactivated. Returning to menu."));
      return MODE_MENU;
    }
  }

  // ---- RECORDING COMMANDS ----
  else if (input == 'R') {
    // Check for slot number in second character
    if (cmd[1] >= '1' && cmd[1] <= '0' + NUM_RECORDING_SLOTS) {
      int slot_num = cmd[1] - '1';
      if (startRecording(slot_num)) {
        Serial.print(F("\nRecording to Slot "));
        Serial.print(slot_num + 1);
        Serial.println(F("... Play some notes!"));
        Serial.println(F("Press 'S' to stop recording.\n"));
        return MODE_RECORDING;
      }
    } else {
      Serial.println(F("\nUsage: R[1-3] (e.g., R1, R2, R3)"));
    }
  }

  else if (input == 'S') {
    if (stopRecording()) {
      Serial.println(F("\nRecording stopped."));
      int slot = getActiveRecordingSlot();
      if (slot >= 0) {
        Serial.print(F("Saved to Slot "));
        Serial.print(slot + 1);
        Serial.print(F(" ("));
        Serial.print(getSlotNoteCount(slot));
        Serial.println(F(" notes)"));
      }
      Serial.println();
      return MODE_FREE_PLAY;
    } else {
      Serial.println(F("\nNot currently recording."));
    }
  }

  // ---- PLAYBACK COMMANDS ----
  else if (input == 'P') {
    char slot_char = cmd[1];

    // Play all slots
    if (slot_char == 'A' || slot_char == 'a') {
      if (playAllSlots(current_overlap_strategy)) {
        Serial.print(F("\nPlaying all slots ("));
        printOverlapStrategy(current_overlap_strategy);
        Serial.println(F(" mode)..."));
        return MODE_PLAYBACK;
      } else {
        Serial.println(F("\nNo recordings to play."));
      }
    }
    // Play specific slot
    else if (slot_char >= '1' && slot_char <= '0' + NUM_RECORDING_SLOTS) {
      int slot_num = slot_char - '1';
      if (playSingleSlot(slot_num)) {
        Serial.print(F("\nPlaying Slot "));
        Serial.print(slot_num + 1);
        Serial.println(F("..."));
        return MODE_PLAYBACK;
      } else {
        Serial.print(F("\nSlot "));
        Serial.print(slot_num + 1);
        Serial.println(F(" is empty."));
      }
    } else {
      Serial.println(F("\nUsage: P[1-3] or PA (e.g., P1, P2, PA)"));
    }
  }

  else if (input == 'X') {
    stopPlayback();
    Serial.println(F("\nPlayback stopped."));
    return MODE_FREE_PLAY;
  }

  // ---- MANAGEMENT COMMANDS ----
  else if (input == 'L') {
    listRecordings();
  }

  else if (input == 'C') {
    char slot_char = cmd[1];

    // Clear all slots
    if (slot_char == 'A' || slot_char == 'a') {
      clearAllRecordings();
      Serial.println(F("\nAll recordings cleared."));
    }
    // Clear specific slot
    else if (slot_char >= '1' && slot_char <= '0' + NUM_RECORDING_SLOTS) {
      int slot_num = slot_char - '1';
      if (clearRecordingSlot(slot_num)) {
        Serial.print(F("\nSlot "));
        Serial.print(slot_num + 1);
        Serial.println(F(" cleared."));
      }
    } else {
      Serial.println(F("\nUsage: C[1-3] or CA (e.g., C1, C2, CA)"));
    }
  }

  // ---- OVERLAP MODE SELECTION ----
  else if (input == 'M') {
    char mode_char = cmd[1];

    if (mode_char >= '1' && mode_char <= '4') {
      current_overlap_strategy = (OverlapStrategy)(mode_char - '1');
      Serial.print(F("\nOverlap mode set to: "));
      printOverlapStrategy(current_overlap_strategy);
      Serial.println();
    } else {
      Serial.println(F("\nUsage: M[1-4] (M1=High, M2=Low, M3=Alternate, M4=Drop)"));
    }
  }

  // ---- DEBUG COMMANDS ----
  else if (strcmp(cmd, "DBG") == 0) {
    toggleDebugMode();
  }

  else if (strcmp(cmd, "STATUS") == 0) {
    printSystemStatus();
  }

  else if (strcmp(cmd, "STATS") == 0) {
    printDetailedStats();
  }

  else if (strcmp(cmd, "RESET") == 0) {
    resetDebugStats();
  }

  // ---- HELP / INVALID COMMAND ----
  else if (input == 'H' || input == '?') {
    printMainMenu();
  }

  return current_mode;  // Return current mode if no change
}

/**
 * Check and process serial input (reads complete lines)
 * @return Updated system mode
 */
SystemMode processSerialInput() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    // Check for line ending
    if (c == '\n' || c == '\r') {
      if (buffer_index > 0) {
        // Null-terminate the string
        input_buffer[buffer_index] = '\0';

        // Process the command
        SystemMode new_mode = handleSerialCommand(input_buffer);

        // Reset buffer for next command
        buffer_index = 0;

        return new_mode;
      }
      // Empty line, ignore
      continue;
    }

    // Add character to buffer (if there's space)
    if (buffer_index < INPUT_BUFFER_SIZE - 1) {
      input_buffer[buffer_index++] = c;
    } else {
      // Buffer overflow - reset and warn
      Serial.println(F("\nError: Command too long!"));
      buffer_index = 0;
    }
  }

  return current_mode;
}

#endif // UI_H
