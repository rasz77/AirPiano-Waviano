#ifndef RECORDING_H
#define RECORDING_H

#include <Arduino.h>
#include "config.h"
#include "note_mapping.h"

// ============================================
// RECORDING DATA STRUCTURES
// ============================================

/**
 * Represents a single note event in a recording
 */
struct NoteEvent {
  uint8_t note_index;        // Note index (0-7)
  uint8_t duration_units;    // Duration in DURATION_UNIT_MS units (100ms each)

  NoteEvent() : note_index(0), duration_units(0) {}

  NoteEvent(uint8_t note, uint8_t duration)
    : note_index(note), duration_units(duration) {}
};

/**
 * Represents a recording slot
 */
struct RecordingSlot {
  NoteEvent events[MAX_NOTES_PER_SLOT];  // Array of note events
  int note_count;                         // Number of notes in this recording
  bool is_active;                         // Whether this slot contains a recording

  RecordingSlot() : note_count(0), is_active(false) {}
};

// ============================================
// RECORDING STATE
// ============================================

// Array of recording slots
RecordingSlot recording_slots[NUM_RECORDING_SLOTS];

// Recording state
bool is_recording = false;
int active_recording_slot = -1;
unsigned long recording_start_time = 0;
unsigned long last_note_time = 0;
int last_note_index = -1;

// ============================================
// RECORDING MANAGEMENT FUNCTIONS
// ============================================

/**
 * Initialize recording system
 */
void initializeRecordingSystem() {
  for (int i = 0; i < NUM_RECORDING_SLOTS; i++) {
    recording_slots[i].note_count = 0;
    recording_slots[i].is_active = false;
  }
  is_recording = false;
  active_recording_slot = -1;
}

/**
 * Start recording to a specific slot
 * @param slot_num Slot number (0 to NUM_RECORDING_SLOTS-1)
 * @return true if recording started successfully
 */
bool startRecording(int slot_num) {
  if (slot_num < 0 || slot_num >= NUM_RECORDING_SLOTS) {
    return false;  // Invalid slot
  }

  if (is_recording) {
    return false;  // Already recording
  }

  // Clear the slot
  recording_slots[slot_num].note_count = 0;
  recording_slots[slot_num].is_active = false;

  // Start recording
  is_recording = true;
  active_recording_slot = slot_num;
  recording_start_time = millis();
  last_note_time = recording_start_time;
  last_note_index = -1;

  return true;
}

/**
 * Stop current recording
 * @return true if recording was stopped successfully
 */
bool stopRecording() {
  if (!is_recording) {
    return false;  // Not recording
  }

  // Finalize the last note if there was one
  if (last_note_index != -1 && active_recording_slot >= 0) {
    unsigned long current_time = millis();
    unsigned long duration_ms = current_time - last_note_time;
    uint8_t duration_units = duration_ms / DURATION_UNIT_MS;

    if (duration_units > MAX_NOTE_DURATION_UNITS) {
      duration_units = MAX_NOTE_DURATION_UNITS;
    }

    // Update the last note's duration
    RecordingSlot* slot = &recording_slots[active_recording_slot];
    if (slot->note_count > 0) {
      slot->events[slot->note_count - 1].duration_units = duration_units;
    }
  }

  // Mark slot as active if it has notes
  if (active_recording_slot >= 0 && recording_slots[active_recording_slot].note_count > 0) {
    recording_slots[active_recording_slot].is_active = true;
  }

  is_recording = false;
  active_recording_slot = -1;
  last_note_index = -1;

  return true;
}

/**
 * Add a note to the current recording
 * @param note_index Note index (0-7)
 * @return true if note was added successfully
 */
bool addNoteToRecording(int note_index) {
  if (!is_recording || active_recording_slot < 0) {
    return false;  // Not recording
  }

  if (note_index < 0 || note_index >= NUM_NOTES) {
    return false;  // Invalid note
  }

  RecordingSlot* slot = &recording_slots[active_recording_slot];

  unsigned long current_time = millis();

  // If this is a different note than the last one, save the previous note
  if (last_note_index != -1 && last_note_index != note_index) {
    // Calculate duration of the previous note
    unsigned long duration_ms = current_time - last_note_time;
    uint8_t duration_units = duration_ms / DURATION_UNIT_MS;

    if (duration_units == 0) {
      duration_units = 1;  // Minimum duration
    }
    if (duration_units > MAX_NOTE_DURATION_UNITS) {
      duration_units = MAX_NOTE_DURATION_UNITS;
    }

    // Update the last note's duration
    if (slot->note_count > 0) {
      slot->events[slot->note_count - 1].duration_units = duration_units;
    }

    // Add new note
    if (slot->note_count < MAX_NOTES_PER_SLOT) {
      slot->events[slot->note_count] = NoteEvent(note_index, 0);
      slot->note_count++;
      last_note_time = current_time;
      last_note_index = note_index;
    } else {
      // Buffer full - stop recording
      stopRecording();
      return false;
    }
  } else if (last_note_index == -1) {
    // First note in recording
    if (slot->note_count < MAX_NOTES_PER_SLOT) {
      slot->events[slot->note_count] = NoteEvent(note_index, 0);
      slot->note_count++;
      last_note_time = current_time;
      last_note_index = note_index;
    }
  }
  // If same note as last, just extend its duration (will be calculated when next note comes)

  return true;
}

/**
 * Clear a recording slot
 * @param slot_num Slot number (0 to NUM_RECORDING_SLOTS-1)
 * @return true if slot was cleared successfully
 */
bool clearRecordingSlot(int slot_num) {
  if (slot_num < 0 || slot_num >= NUM_RECORDING_SLOTS) {
    return false;
  }

  recording_slots[slot_num].note_count = 0;
  recording_slots[slot_num].is_active = false;

  return true;
}

/**
 * Clear all recording slots
 */
void clearAllRecordings() {
  for (int i = 0; i < NUM_RECORDING_SLOTS; i++) {
    clearRecordingSlot(i);
  }
}

/**
 * Check if currently recording
 * @return true if recording is active
 */
bool isRecording() {
  return is_recording;
}

/**
 * Get the active recording slot number
 * @return Slot number, or -1 if not recording
 */
int getActiveRecordingSlot() {
  return active_recording_slot;
}

/**
 * Check if a slot has a recording
 * @param slot_num Slot number
 * @return true if slot has a recording
 */
bool isSlotActive(int slot_num) {
  if (slot_num < 0 || slot_num >= NUM_RECORDING_SLOTS) {
    return false;
  }
  return recording_slots[slot_num].is_active && recording_slots[slot_num].note_count > 0;
}

/**
 * Get recording slot data
 * @param slot_num Slot number
 * @return Pointer to recording slot, or NULL if invalid
 */
RecordingSlot* getRecordingSlot(int slot_num) {
  if (slot_num < 0 || slot_num >= NUM_RECORDING_SLOTS) {
    return NULL;
  }
  return &recording_slots[slot_num];
}

/**
 * Get number of notes in a slot
 * @param slot_num Slot number
 * @return Number of notes, or -1 if invalid slot
 */
int getSlotNoteCount(int slot_num) {
  if (slot_num < 0 || slot_num >= NUM_RECORDING_SLOTS) {
    return -1;
  }
  return recording_slots[slot_num].note_count;
}

/**
 * Get total duration of a recording in milliseconds
 * @param slot_num Slot number
 * @return Total duration in ms, or 0 if invalid
 */
unsigned long getRecordingDuration(int slot_num) {
  RecordingSlot* slot = getRecordingSlot(slot_num);
  if (slot == NULL || !slot->is_active) {
    return 0;
  }

  unsigned long total_duration = 0;
  for (int i = 0; i < slot->note_count; i++) {
    total_duration += slot->events[i].duration_units * DURATION_UNIT_MS;
  }

  return total_duration;
}

#endif // RECORDING_H
