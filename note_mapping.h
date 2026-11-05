#ifndef NOTE_MAPPING_H
#define NOTE_MAPPING_H

#include "config.h"

// ============================================
// MUSICAL NOTE DEFINITIONS
// ============================================

// Number of notes supported
#define NUM_NOTES 8

// Note frequencies (8 notes: Do Re Mi Fa Sol La Si Do)
const int note_frequencies[NUM_NOTES] = {
  523,  // Do (C5)
  587,  // Re (D5)
  659,  // Mi (E5)
  698,  // Fa (F5)
  784,  // Sol (G5)
  880,  // La (A5)
  988,  // Si (B5)
  1046  // Do (C6)
};

// Note names for display
const char* note_names[NUM_NOTES] = {
  "Do (C5)",
  "Re (D5)",
  "Mi (E5)",
  "Fa (F5)",
  "Sol (G5)",
  "La (A5)",
  "Si (B5)",
  "Do (C6)"
};

// Short note names
const char* note_names_short[NUM_NOTES] = {
  "Do", "Re", "Mi", "Fa", "Sol", "La", "Si", "Do*"
};

// ============================================
// DISTANCE TO NOTE MAPPING
// ============================================

// Distance ranges for each note (in cm)
struct DistanceRange {
  float min_cm;
  float max_cm;
};

const DistanceRange distance_ranges[NUM_NOTES] = {
  {2.0,  10.0},   // Do (C5)
  {10.0, 20.0},   // Re (D5)
  {20.0, 30.0},   // Mi (E5)
  {30.0, 40.0},   // Fa (F5)
  {40.0, 50.0},   // Sol (G5)
  {50.0, 60.0},   // La (A5)
  {60.0, 70.0},   // Si (B5)
  {70.0, 80.0}    // Do (C6)
};

// ============================================
// NOTE MAPPING FUNCTIONS
// ============================================

/**
 * Get note index from distance measurement
 * @param distance_cm Distance in centimeters
 * @return Note index (0-7) or -1 if out of range
 */
int getNoteFromDistance(float distance_cm) {
  for (int i = 0; i < NUM_NOTES; i++) {
    if (distance_cm > distance_ranges[i].min_cm &&
        distance_cm <= distance_ranges[i].max_cm) {
      return i;
    }
  }
  return -1;  // Out of range
}

/**
 * Get frequency for a note index
 * @param note_index Note index (0-7)
 * @return Frequency in Hz, or 0 if invalid
 */
int getNoteFrequency(int note_index) {
  if (note_index >= 0 && note_index < NUM_NOTES) {
    return note_frequencies[note_index];
  }
  return 0;
}

/**
 * Get LED bit position for a note index (for shift register)
 * @param note_index Note index (0-7)
 * @return Bit position (0-7) or -1 if invalid
 * Note: With shift register, bit 0 = Do, bit 1 = Re, ..., bit 7 = Do (High)
 */
int getNoteLED(int note_index) {
  if (note_index >= 0 && note_index < NUM_NOTES) {
    return note_index;  // Direct mapping to shift register bits
  }
  return -1;  // Invalid
}

/**
 * Get note name for display
 * @param note_index Note index (0-7)
 * @param short_form Use short name (true) or long name (false)
 * @return Note name string
 */
const char* getNoteName(int note_index, bool short_form = false) {
  if (note_index >= 0 && note_index < NUM_NOTES) {
    return short_form ? note_names_short[note_index] : note_names[note_index];
  }
  return "---";
}

#endif // NOTE_MAPPING_H
