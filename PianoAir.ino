/**
 * PianoAir - Air Piano with Ultrasonic Sensor
 *
 * Features:
 * - Free play mode: Play any notes freely
 * - Recording: Record your performances to 4 slots
 * - Multi-track playback: Play back recordings individually or merged
 * - IR Remote control: Control all functions with IR remote
 *
 * Hardware:
 * - Ultrasonic sensor (HC-SR04)
 * - 8 LEDs for note indication
 * - Buzzer for sound output
 * - IR receiver module (pin 11)
 *
 * Version 2.0: Discrete-Time Simulation Architecture
 * - Non-blocking audio state machine
 * - Time-sliced task scheduler
 * - Queue-based IR command processing
 * - Guaranteed IR polling at 100Hz
 */

#include <IRremote.h>
#include "config.h"
#include "note_mapping.h"
#include "utils.h"
#include "audio_state.h"    // NEW: Non-blocking audio
#include "recording.h"
#include "playback.h"
#include "ui.h"
#include "ir_remote.h"
#include "debug.h"          // NEW: Debug system

// ============================================
// GLOBAL STATE
// ============================================

// Current system mode
SystemMode current_mode = MODE_MENU;

// Currently selected recording slot for recording
int current_recording_slot = -1;

// Overlap strategy for multi-track playback
OverlapStrategy overlap_strategy = DEFAULT_OVERLAP_STRATEGY;

// Last detected note for debouncing
int last_detected_note = -1;
unsigned long last_detected_note_time = 0;

// ============================================
// SETUP
// ============================================

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize hardware (pins, interrupts)
  initializeHardware();

  // Initialize Timer1 for buzzer 2 (true dual-tone capability)
  setupTimer1();

  // Initialize recording system
  initializeRecordingSystem();

  // Initialize IR receiver
  initIRReceiver();

  // Initialize debug system
  initDebugSystem();

  // Print welcome message and menu
  Serial.println(F("\n\n"));
  Serial.println(F("*****************************************"));
  Serial.println(F("*                                       *"));
  Serial.println(F("*         WELCOME TO PIANO AIR!         *"));
  Serial.println(F("*                                       *"));
  Serial.println(F("*****************************************"));

  printMainMenu();
  printIRButtonReference();

  // Show debug status
  if (debug_enabled) {
    Serial.println(F("\n*** DEBUG MODE: VERBOSE ***"));
    Serial.println(F("Commands: DBG, STATUS, STATS, RESET"));
  }
}

// ============================================
// MAIN LOOP - DISCRETE-TIME SIMULATION
// ============================================

void loop() {
  static unsigned long loop_start = 0;
  static unsigned long last_ir_poll = 0;
  static unsigned long last_audio_update = 0;
  static unsigned long last_loop_warning = 0;

  loop_start = millis();

  // ===== CRITICAL: IR PROCESSING (HIGHEST PRIORITY) =====
  // Poll IR receiver at guaranteed 100Hz rate
  if (millis() - last_ir_poll >= IR_POLL_INTERVAL_MS) {
    processIRTask();  // Fast decode and queue
    last_ir_poll = millis();
  }

  // Process queued IR commands (one per cycle)
  SystemMode new_mode = processQueuedIRCommands();
  if (new_mode != current_mode) {
    debugModeChange(current_mode, new_mode);
    current_mode = new_mode;
  }

  // ===== SERIAL INPUT PROCESSING =====
  new_mode = processSerialInput();
  if (new_mode != current_mode) {
    debugModeChange(current_mode, new_mode);
    current_mode = new_mode;
  }

  // ===== AUDIO STATE MACHINE UPDATE =====
  // Update at high frequency for smooth volume pulsing
  if (millis() - last_audio_update >= AUDIO_UPDATE_INTERVAL_MS) {
    updateAudioStateMachine();
    last_audio_update = millis();
  }

  // ===== ULTRASONIC SENSOR UPDATE =====
  // Update in all modes except menu (allows playing along with playback!)
  if (current_mode != MODE_MENU) {
    updateUltrasonicSensor();
  }

  // ===== PLAYBACK UPDATE (NON-BLOCKING) =====
  if (current_mode == MODE_PLAYBACK) {
    if (!updatePlayback()) {
      // Playback finished
      current_mode = MODE_FREE_PLAY;
      Serial.println(F("\nPlayback finished.\n"));
    }
    // CRITICAL FIX: Do NOT return early!
    // Continue processing below to maintain IR polling
  }

  // ===== PROCESS SENSOR INPUT (NON-BLOCKING) =====
  if ((current_mode == MODE_FREE_PLAY || current_mode == MODE_RECORDING || current_mode == MODE_PLAYBACK) &&
      isNewDistanceAvailable()) {

    clearDistanceFlag();
    float distance = getDistance();
    int note_index = getNoteFromDistance(distance);

    if (note_index != -1) {
      unsigned long now = millis();
      bool is_new_note = (note_index != last_detected_note) ||
                         (now - last_detected_note_time > NOTE_DEBOUNCE_MS);

      if (is_new_note) {
        last_detected_note = note_index;
        last_detected_note_time = now;

        // Use non-blocking audio for all notes
        if (current_mode == MODE_FREE_PLAY || current_mode == MODE_PLAYBACK) {
          requestPlayNote(note_index, NOTE_DURATION_MS);
          #if ENABLE_DEBUG
          Serial.print(F("Note: "));
          Serial.println(getNoteName(note_index, true));
          #endif
        } else if (current_mode == MODE_RECORDING) {
          if (addNoteToRecording(note_index)) {
            requestPlayNote(note_index, NOTE_DURATION_MS);
            #if ENABLE_DEBUG
            int slot = getActiveRecordingSlot();
            Serial.print(F("Recorded: "));
            Serial.print(getNoteName(note_index, true));
            Serial.print(F(" ["));
            Serial.print(getSlotNoteCount(slot));
            Serial.print(F("/"));
            Serial.print(MAX_NOTES_PER_SLOT);
            Serial.println(F("]"));
            #endif
          } else {
            // Recording buffer full
            Serial.println(F("\n*** Recording buffer full! Recording stopped. ***\n"));
            stopRecording();
            current_mode = MODE_FREE_PLAY;
          }
        }
      }
    }
  }

  // ===== MAINTAIN CONSISTENT LOOP TIMING =====
  unsigned long loop_duration = millis() - loop_start;

  // Update loop statistics
  updateLoopStats(loop_duration);

  // Warn if loop takes too long (affects responsiveness)
  if (loop_duration > MAX_LOOP_DURATION_MS) {
    if (millis() - last_loop_warning > 5000) {  // Warn every 5 seconds max
      if (!debug_enabled) {  // Only print if debug not already printing
        Serial.print(F("[WARNING] Loop overrun: "));
        Serial.print(loop_duration);
        Serial.println(F("ms"));
      }
      last_loop_warning = millis();
    }
  }

  // Minimal delay to prevent tight spinning and save power
  if (loop_duration < SCHEDULER_TICK_MS) {
    delay(SCHEDULER_TICK_MS - loop_duration);
  }
}

// Note handling functions removed - using inline code instead for memory savings
